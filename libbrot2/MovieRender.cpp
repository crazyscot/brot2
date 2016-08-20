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

Movie::RenderJob::RenderJob(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, Movie::Renderer& renderer, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const char* argv0) :
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

void Movie::Renderer::start(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const char* argv0) {
	cancel_requested = false;
	std::shared_ptr<RenderJob> job (new RenderJob(reporter, parent, *this, filename, movie, prefs, threads, argv0));
	threads.enqueue<void>([=]{ job->run(); });
}

void Movie::Renderer::do_blocking(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const char* argv0) {
	cancel_requested = false;
	std::shared_ptr<RenderJob> job (new RenderJob(reporter, parent, *this, filename, movie, prefs, threads, argv0));
	job->run();
}

void Movie::Renderer::render(RenderJob* job) {
	RenderInstancePrivate * priv;
	const struct Movie::MovieInfo& movie(job->_movie);
	render_top(*job, &priv);
	// Sanity check - trap incomplete structs
	if (!movie.points.size()) {
		render_tail(priv);
		return;
	}
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
		if (!ok) {
			render_tail(priv);
			return;
		}
	}

	auto iter = movie.points.begin();

	struct Movie::KeyFrame f1(*iter);
	render_frame(f1, priv, f1.hold_frames+1); // +1 as we must output the initial frame once, even if we are not holding on it. We expect that render_frame will call plot_complete but not frames_traversed.
	job->_reporter->frames_traversed(f1.hold_frames);
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

		// We've just output a single frame of the destination keyframe.
		if (f2.hold_frames) {
			render_frame(f2, priv, f2.hold_frames); // we expect this will call plot_complete once but not frames_traversed
			job->_reporter->frames_traversed(f2.hold_frames-1);
		}

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
// And now some instances to make them live

MOVIERENDER_DECLARE_FACTORY(ScriptB2CLI, "Script for brot2cli", "*.sh");
