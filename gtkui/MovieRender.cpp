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

#include "MovieRender.h"
#include "MovieMode.h"
#include "MovieMotion.h"
#include "MovieProgress.h"
#include "misc.h"
#include "gtkutil.h"
#include "SaveAsPNG.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <errno.h>

using namespace std;

Movie::Renderer::Renderer(const std::string& _name, const std::string& _pattern) : cancel_requested(false), name(_name), pattern(_pattern) {
}
Movie::Renderer::~Renderer() {
}

SimpleRegistry<Movie::RendererFactory> Movie::RendererFactory::all_factories;

Movie::RendererFactory::RendererFactory(const std::string& _name, const std::string& _pattern) : name(_name), pattern(_pattern) {
	all_factories.reg(_name, this);
}

Movie::RendererFactory::~RendererFactory() {
	all_factories.dereg(name);
}
/*STATIC*/
Movie::RendererFactory* Movie::RendererFactory::get_factory(const std::string& name) {
	return all_factories.get(name);
}
/*STATIC*/
std::set<std::string> Movie::RendererFactory::all_factory_names() {
	return all_factories.names();
}

// ---------------------------------------------------------------------

Movie::RenderInstancePrivate::RenderInstancePrivate(Movie::RenderJob& _job) : job(_job)
{
}
Movie::RenderInstancePrivate::~RenderInstancePrivate()
{
}

// ---------------------------------------------------------------------

Movie::RenderJob::RenderJob(IRenderProgressReporter& reporter, IRenderCompleteHandler& parent, Movie::Renderer& renderer, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const char* argv0) :
	_parent(parent),
	_reporter(&reporter),
	_renderer(renderer),
	_filename(filename), _movie(movie), _prefs(prefs), _threads(threads), _argv0(argv0) {
}

void Movie::RenderJob::run() {
	_renderer.render(this);
	_parent.signal_completion(*this);
}
Movie::RenderJob::~RenderJob() {
}

void Movie::Renderer::start(IRenderProgressReporter& reporter, IRenderCompleteHandler& parent, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const char* argv0) {
	cancel_requested = false;
	std::shared_ptr<RenderJob> job (new RenderJob(reporter, parent, *this, filename, movie, prefs, threads, argv0));
	threads.enqueue<void>([=]{ job->run(); });
}

void Movie::Renderer::render(RenderJob* job) {
	RenderInstancePrivate * priv;
	const struct Movie::MovieInfo& movie(job->_movie);
	render_top(*job, &priv);

	auto iter = movie.points.begin();

	// NOTE: The total number of frames we render should match the number calculated by MovieWindow::do_update_duration().
	// TODO Have do_update_duration call into here for a frame count.

	struct Movie::KeyFrame f1(*iter);
	if (f1.hold_frames)
		render_frame(f1, priv, f1.hold_frames); // we expect this will call plot_complete but not frames_traversed
	else
		render_frame(f1, priv, 1);

	job->_reporter->frames_traversed(f1.hold_frames); // FIXME Check semantics, is this call correct?
	iter++;

	for (; iter != movie.points.end() && !cancel_requested; iter++) {
		struct Movie::Frame ft(f1);
		struct Movie::KeyFrame f2(*iter);

		bool still_moving;
		do {
			still_moving = false;
			still_moving |= Movie::MotionZoom(ft.size, f2.size, movie.width, movie.height, f1.speed_zoom, ft.size);
			still_moving |= Movie::MotionTranslate(ft.centre, f2.centre, ft.size, movie.width, movie.height, f1.speed_translate, ft.centre);
			if (still_moving)
				render_frame(ft, priv, 1);
		} while (still_moving && !cancel_requested);

		if (f1.hold_frames>1)
			render_frame(f2, priv, f1.hold_frames-1); // we expect this will call plot_complete but not frames_traversed
		job->_reporter->frames_traversed(f1.hold_frames); // FIXME Check semantics, is this call correct?

		f1 = f2;
	}
	render_tail(priv);
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
				<< "${BROT2CLI} -X " << fr.centre.real() << " -Y " << fr.centre.imag()
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
			mypriv->fs << endl;

			for (unsigned i=1; i<n_frames; i++) {
				mypriv->fs
					<< "cp " << filename.str() << " "
					<< "\"${OUTDIR}/${OUTNAME}." << setfill('0') << setw(6) << (mypriv->fileno++) << ".png\"" 
					<< endl;
			}
		}
		void render_tail(Movie::RenderInstancePrivate *priv, bool) {
			Private * mypriv = (Private*)(priv);
			delete mypriv;
		}
		virtual ~ScriptB2CLI() {}
};

// ------------------------------------------------------------------------------
// Outputs a bunch of PNGs in a single directory

class BunchOfPNGs : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class BunchOfPNGs;
		unsigned fileno;
		const std::string outdir, nametmpl;

		Private(Movie::RenderJob& _job, const std::string& _outdir, const std::string& _tmpl) :
			RenderInstancePrivate(_job), fileno(0), outdir(_outdir), nametmpl(_tmpl)
		{
		}
		virtual ~Private() {}
	};
	public:
		BunchOfPNGs() : Movie::Renderer("PNG files in a directory", "*.png") { }

		void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate **priv) {
			std::string outdir(job._filename), tmpl(job._filename);
			int spos = outdir.rfind('/');
			if (spos >= 0) {
				outdir.erase(spos+1);
				tmpl.erase(0, spos+1);
			}
			spos = tmpl.rfind(".png");
			if (spos >= 0)
				tmpl.erase(spos);
			Private *mypriv = new Private(job, outdir, tmpl);
			*priv = mypriv;
		}
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv, unsigned n_frames) {
			Private * mypriv = (Private*)(priv);
			ostringstream filename;
			filename << mypriv->outdir << '/' << mypriv->nametmpl << '_' << setfill('0') << setw(6) << (mypriv->fileno++) << ".png";
			std::string filename2 (filename.str());

			SavePNG::MovieFrame saver(mypriv->job._prefs, mypriv->job._threads,
					*mypriv->job._movie.fractal, *mypriv->job._movie.palette,
					*mypriv->job._reporter,
					fr.centre, fr.size,
					mypriv->job._movie.width, mypriv->job._movie.height,
					mypriv->job._movie.antialias, mypriv->job._movie.draw_hud,
					filename2);
			saver.start();
			mypriv->job._reporter->set_chunks_count(saver.get_chunks_count());
			saver.wait();
			saver.save_png(0);
			for (unsigned i=1; i<n_frames; i++) {
				ostringstream file_copy;
				file_copy << mypriv->outdir << '/' << mypriv->nametmpl << '_' << setfill('0') << setw(6) << (mypriv->fileno++) << ".png";
				if (Util::copy_file(filename2, file_copy.str()) != 0) {
					ostringstream msg;
					msg << "Error copying frames: " << strerror(errno);
					Util::alert(0, msg.str());
				}
			}
			// LATER Could kick off all frames in parallel and wait for them in render_bottom?
		}
		void render_tail(Movie::RenderInstancePrivate *priv, bool) {
			Private * mypriv = (Private*)(priv);
			delete mypriv;
		}
		virtual ~BunchOfPNGs() {}
};

// ------------------------------------------------------------------------------
// And now some instances to make them live

MOVIERENDER_DECLARE_FACTORY(ScriptB2CLI, "Script for brot2cli", "*.sh");
MOVIERENDER_DECLARE_FACTORY(BunchOfPNGs, "PNG files in a directory", "*.png");
