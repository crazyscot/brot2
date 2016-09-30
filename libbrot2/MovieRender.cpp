/*
    MovieRender.cpp: brot2 movie rendering
    Copyright (C) 2016 Ross Younger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "IMovieProgress.h"
#include "MovieRender.h"
#include "MovieMode.h"
#include "MovieMotion.h"
#include "misc.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <errno.h>

using namespace std;

ThreadPool Movie::Renderer::movie_runner_thread(1);

Movie::Renderer::Renderer(const std::string& _name, const std::string& _pattern) : cancel_requested(false), name(_name), pattern(_pattern) {
}
Movie::Renderer::~Renderer() {
}

SimpleRegistry<Movie::RendererFactory>* Movie::RendererFactory::all_factories() {
	static SimpleRegistry<Movie::RendererFactory>* all_factories_ = new SimpleRegistry<Movie::RendererFactory>();
	return all_factories_;
}

Movie::RendererFactory::RendererFactory(const std::string& _name, const std::string& _pattern) : name(_name), pattern(_pattern) {
	all_factories()->reg(_name, this);
}

Movie::RendererFactory::~RendererFactory() {
	all_factories()->dereg(name);
}
/*STATIC*/
Movie::RendererFactory* Movie::RendererFactory::get_factory(const std::string& name) {
	return all_factories()->get(name);
}
/*STATIC*/
std::set<std::string> Movie::RendererFactory::all_factory_names() {
	return all_factories()->names();
}

// ---------------------------------------------------------------------

Movie::RenderInstancePrivate::RenderInstancePrivate(Movie::RenderJob& _job) : job(_job)
{
}
Movie::RenderInstancePrivate::~RenderInstancePrivate()
{
}

// ---------------------------------------------------------------------

Movie::RenderJob::RenderJob(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, Movie::Renderer& renderer, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> threads, const char* argv0) :
	_parent(parent),
	_reporter(&reporter),
	_renderer(renderer),
	_filename(filename), _movie(movie), _prefs(prefs), _threads(threads), _argv0(argv0),
	_rwidth( movie.width * (movie.antialias ? 2 : 1) ),
	_rheight( movie.height * (movie.antialias ? 2 : 1) ) {
}

void Movie::RenderJob::run() {
	RenderInstancePrivate * priv(0);
	try {
		_renderer.render_top(*this, &priv); // allocs priv of desired subclass
		ASSERT(priv != 0);
		_renderer.render(priv);
		_renderer.render_tail(priv); // Flush file, delete anything that the destructor doesn't catch
	} catch (BrotException e) {
		_parent.signal_error(*this, e.msg);
	}
	delete priv;
	_parent.signal_completion(*this);
}
Movie::RenderJob::~RenderJob() {
}

void Movie::Renderer::start(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> worker_threads, const char* argv0) {
	cancel_requested = false;
	std::shared_ptr<RenderJob> job (new RenderJob(reporter, parent, *this, filename, movie, prefs, worker_threads, argv0));
	movie_runner_thread.enqueue<void>([=]{ job->run(); });
}

void Movie::Renderer::do_blocking(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> threads, const char* argv0) {
	cancel_requested = false;
	std::shared_ptr<RenderJob> job (new RenderJob(reporter, parent, *this, filename, movie, prefs, threads, argv0));
	job->run();
}

void Movie::Renderer::render(RenderInstancePrivate *priv) {
	Movie::RenderJob* job = &priv->job;
	const struct Movie::MovieInfo& movie(job->_movie);
	// Sanity check - trap incomplete structs
	if (!movie.points.size())
		return;
	{
		bool ok = true;
		for (auto iter = movie.points.begin(); iter != movie.points.end(); iter++) {
			// Size 0 cannot be rendered
			ok &= (real((*iter).size) != 0);
			ok &= (imag((*iter).size) != 0);
			// Speed 0 cannot be rendered
			ok &= ((*iter).speed_zoom != 0);
			ok &= ((*iter).speed_translate != 0);
		}
		if (!ok)
			return;
	}

	auto iter = movie.points.begin();

	struct Movie::KeyFrame f1(*iter);
	render_frame(f1, priv, f1.hold_frames+1); // +1 as we must output the initial frame once, even if we are not holding on it. We expect that render_frame will call plot_complete but not frames_traversed.
	job->_reporter->frames_traversed(f1.hold_frames);
	iter++;

	bool skip_next = false;

	for (; iter != movie.points.end() && !cancel_requested; iter++) {
		struct Movie::Frame ft(f1);
		struct Movie::KeyFrame f2(*iter);

		bool still_moving;
		do {
			still_moving = false;
			still_moving |= Movie::MotionZoom(ft.size, f2.size, movie.width, movie.height, f1.speed_zoom, ft.size);
			still_moving |= Movie::MotionTranslate(ft.centre, f2.centre, ft.size, movie.width, movie.height, f1.speed_translate, ft.centre);
			if (still_moving && !skip_next)
				render_frame(ft, priv, 1);
			if (movie.preview)
				skip_next = !skip_next;
		} while (still_moving && !cancel_requested);

		// We've just output a single frame of the destination keyframe.
		if (f2.hold_frames) {
			render_frame(f2, priv, f2.hold_frames); // we expect this will call plot_complete once but not frames_traversed
			job->_reporter->frames_traversed(f2.hold_frames-1);
		}

		f1 = f2;
	}
}

void Movie::Renderer::request_cancel() {
	cancel_requested = true;
}

// ------------------------------------------------------------------------------
// Outputs a script to drive brot2cli

class ScriptB2CLI : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class ScriptB2CLI;
		ofstream fs;
		unsigned fileno;

		Private(Movie::RenderJob& _job) : Movie::RenderInstancePrivate(_job), fileno(0) {
			fs.open(_job._filename, std::fstream::out);
		}
		~Private() {
			fs.close();
		}
	};
	public:
		ScriptB2CLI() : Movie::Renderer("Script for brot2cli", "*.sh") { }

		void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate **priv) {
			std::string b2cli(job._argv0);
			b2cli.append("cli");
			std::string outdir(job._filename);
			int spos = outdir.rfind('/');
			if (spos >= 0)
				outdir.erase(spos+1);
			Private *mypriv = new Private(job);
			*priv = mypriv;
			mypriv->fs
				<< "#!/bin/bash -x" << endl
				<< "# brot2 generated movie script" << endl
				<< endl
				<< "BROT2CLI=\"" << b2cli << "\"" << endl
				<< "OUTDIR=\"" << outdir << "\"" << endl
				<< "OUTNAME=\"render\"" << endl
				<< endl
				;
		}
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv, const unsigned n_frames) {
			Private * mypriv = (Private*)(priv);
			ostringstream filename;
			filename << "\"${OUTDIR}/${OUTNAME}." << setfill('0') << setw(6) << (mypriv->fileno++) << ".png\"";

			mypriv->fs
				<< "${BROT2CLI}"
				<< " -X " << std::setprecision(Fractal::precision_for(fr.size.real(),mypriv->job._movie.width)) << fr.centre.real()
				<< " -Y " << std::setprecision(Fractal::precision_for(fr.size.imag(),mypriv->job._movie.height)) << fr.centre.imag()
				<< " -l " << fr.size.real()
				<< " -f \"" << mypriv->job._movie.fractal->name << "\""
				<< " -p \"" << mypriv->job._movie.palette->name << "\""
				<< " -h " << mypriv->job._movie.height
				<< " -w " << mypriv->job._movie.width
				<< " -o " << filename.str()
				;
			if (mypriv->job._movie.draw_hud)
				mypriv->fs << " -H";
			if (mypriv->job._movie.antialias)
				mypriv->fs << " -a";
			if (mypriv->job._movie.preview)
				mypriv->fs << " --upscale";
			mypriv->fs << endl;

			for (unsigned i=1; i<n_frames; i++) {
				mypriv->fs
					<< "cp " << filename.str() << " "
					<< "\"${OUTDIR}/${OUTNAME}." << setfill('0') << setw(6) << (mypriv->fileno++) << ".png\"" 
					<< endl;
			}
		}
		void render_tail(Movie::RenderInstancePrivate *, bool) {
		}
		virtual ~ScriptB2CLI() {}
};

// ------------------------------------------------------------------------------
// Outputs a script to drive brot2cli via GNU parallel

class ParallelB2CLI : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class ParallelB2CLI;
		ofstream fs;
		unsigned fileno;
		const std::string remote_b2cli, filepart;
		vector<std::string> holdcopies;

		Private(Movie::RenderJob& _job, const std::string& _b2cli, const std::string& _filepart) : Movie::RenderInstancePrivate(_job), fileno(0), remote_b2cli(_b2cli), filepart(_filepart), holdcopies() {
			fs.open(_job._filename, std::fstream::out);
		}
		~Private() {
			fs.close();
		}
	};
	public:
		ParallelB2CLI() : Movie::Renderer("GNU Parallel script for brot2cli", "*.sh") { }

		/* This outputs a shell script that invokes parallel and provides it with data.
		 * Per-frame format:
		 *   FILENO  RealCentre(-X)  ImagCentre(-Y)  RealAxisLength(-l)
		 */

		void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate **priv) {
			std::string parallel = "parallel"; // Could make a pref
			std::string parallel_extra_args = "--cleanup --controlmaster --sshlogin .. --bar --resume-failed"; // Could make a pref
			// --bar works with zenity, see man parallel_tutorial.
			std::string remote_b2cli = "brot2cli"; // Could make this a pref ?

			// What's our filename prefix?
			std::string filepart(job._filename);
			// Strip the directory...
			int spos = filepart.rfind('/');
			if (spos >= 0)
				filepart.erase(0, spos+1);
			// Strip the extension...
			spos = filepart.rfind('.');
			if (spos >= 0)
				filepart.erase(spos);

			Private *mypriv = new Private(job, remote_b2cli, filepart);
			*priv = mypriv;
			mypriv->fs
				<< "#!/bin/sh -e" << endl
				<< "# Generated by brot2" << endl
				<< endl
				<< parallel
				<< " --colsep ' '"
			    << " --return \"" << mypriv->filepart << ".{1}.png\""
			    << " --joblog \"" << mypriv->filepart << ".joblog\""
				<< " " << parallel_extra_args
				<< " " << remote_b2cli
					   << " -X {2} -Y {3} -l {4}"
					   << " -f \\\"" << mypriv->job._movie.fractal->name << "\\\""
					   << " -p \\\"" << mypriv->job._movie.palette->name << "\\\""
					   << " -h " << mypriv->job._movie.height
					   << " -w " << mypriv->job._movie.width
					   << " -o \\\"" << mypriv->filepart << ".{1}.png\\\""
					   << " -q";
			if (mypriv->job._movie.draw_hud)
				mypriv->fs << " -H";
			if (mypriv->job._movie.antialias)
				mypriv->fs << " -a";
			if (mypriv->job._movie.preview)
				mypriv->fs << " --upscale";
			mypriv->fs
				<< " <<__EOT__"
				<< endl
				;
		}
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv, const unsigned n_frames) {
			Private * mypriv = (Private*)(priv);
			unsigned srcfile = mypriv->fileno;

			mypriv->fs
				<< setfill('0') << setw(6) << (mypriv->fileno++)
				<< " " << /*-X*/ std::setprecision(Fractal::precision_for(fr.size.real(),mypriv->job._movie.width)) << fr.centre.real()
				<< " " << /*-Y*/ std::setprecision(Fractal::precision_for(fr.size.imag(),mypriv->job._movie.height)) << fr.centre.imag()
				<< " " << /*-l*/ fr.size.real()
				<< endl;

			/* Deal with copies. We build up a list of 'cp' commands to run once all the rendering is complete.
			 * Don't like the vector<string> holding shell commands? Well, I could have built up a vector<pair<string,vector<string>>> and unpicked it later, but that's too much like a syntactical puzzle generated by a malevolent extraterrestrial species (pace Mickens) for my liking! */
			if (n_frames > 1) {
				std::ostringstream cpcmd;
				// There are multiple ways to do this.
				//    echo 1 2 3 | xargs -n 1 cp 0
				//    parallel -q cp 0 ::: 1 2 3
				//    <0 tee 1 2 3 >/dev/null
				cpcmd << "tee < \"" << mypriv->filepart << "." << setfill('0') << setw(6) << srcfile << ".png\"";
				for (unsigned i=1; i<n_frames; i++) {
					cpcmd << " \"" << mypriv->filepart << "." << setw(6) << (mypriv->fileno++) << ".png\"";
				}
				cpcmd << " > /dev/null";
				mypriv->holdcopies.push_back(cpcmd.str());
			}
		}
		void render_tail(Movie::RenderInstancePrivate *priv, bool) {
			Private * mypriv = (Private*)(priv);
			mypriv->fs << "__EOT__" << endl << endl;

			mypriv->fs << "# Hold frames" << endl;
			for (auto it = mypriv->holdcopies.begin(); it != mypriv->holdcopies.end(); it++) {
				mypriv->fs << (*it) << endl;
			}
			mypriv->fs << "# End of hold frames" << endl << endl;

			mypriv->fs << "echo Render complete. Now running ffmpeg..." << endl
				<< "ffmpeg -i \"" << mypriv->filepart << ".%06d.png\" -c:v libx264 -pix_fmt yuv420p \"" << mypriv->filepart << ".mp4\"" << endl;
		}
		virtual ~ParallelB2CLI() {}
};

// ------------------------------------------------------------------------------
// And now some instances to make them live

MOVIERENDER_DECLARE_FACTORY(ScriptB2CLI, "Script for brot2cli", "*.sh");
MOVIERENDER_DECLARE_FACTORY(ParallelB2CLI, "GNU Parallel script for brot2cli", "*.sh");
