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
#include "MovieProgress.h"
#include "MovieWindow.h"
#include "gtkmain.h" // for argv0
#include "SaveAsPNG.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

SimpleRegistry<Movie::Renderer> Movie::Renderer::all_renderers;

Movie::Renderer::Renderer(const std::string& _name, const std::string& _pattern) : cancel_requested(false), name(_name), pattern(_pattern) {
	all_renderers.reg(_name, this);
}
Movie::Renderer::~Renderer() {
	all_renderers.dereg(name);
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

// ---------------------------------------------------------------------

namespace Movie {
class RenderJob {
	MovieWindow& _parent;
	Movie::Renderer& _renderer;
	const std::string _filename;
	struct Movie::MovieInfo _movie;
	std::shared_ptr<const BrotPrefs::Prefs> _prefs;
	ThreadPool& _threads;

	public:
		RenderJob(MovieWindow& parent, Movie::Renderer& renderer, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads) :
			_parent(parent), _renderer(renderer), _filename(filename), _movie(movie), _prefs(prefs), _threads(threads) { }

		void run() {
			_renderer.render(_filename, _movie, _prefs, _threads);
		}
		virtual ~RenderJob() { }
};

}; // Movie

void Movie::Renderer::start(MovieWindow& parent, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads) {
	cancel_requested = false;
	std::shared_ptr<RenderJob> job (new RenderJob(parent, *this, filename, movie, prefs, threads));
	threads.enqueue<void>([=]{ job->run(); });
}

void Movie::Renderer::render(const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads) {
	RenderInstancePrivate * priv;
	render_top(prefs, threads, filename, movie, &priv);

	auto iter = movie.points.begin();

	struct Movie::Frame f1;
	f1.centre.real(iter->centre.real());
	f1.centre.imag(iter->centre.imag());
	f1.size.real(  iter->size.real());
	f1.size.imag(  iter->size.imag());
	for (unsigned i=0; i<iter->hold_frames; i++)
		render_frame(f1, priv);
	unsigned traverse = iter->frames_to_next;
	iter++;

	for (; iter != movie.points.end() && !cancel_requested; iter++) {
		struct Movie::Frame ft(f1);
		struct Movie::Frame f2;
		f2.centre.real(iter->centre.real());
		f2.centre.imag(iter->centre.imag());
		f2.size.real  (iter->size.real());
		f2.size.imag  (iter->size.imag());

		Fractal::Point step = (f2.centre - f1.centre) / traverse;
		// Simple scaling of the axis length (zoom factor) doesn't work.
		// Looks like it needs to move exponentially from A to B.
		long double scaler = powl ( f2.size.real()/f1.size.real() , 1.0 / traverse );

		if (cancel_requested) break;
		for (unsigned i=0; i<traverse; i++) {
			Fractal::Point stepx(i * step.real(), i * step.imag());
			ft.centre = f1.centre + stepx;
			ft.size = f1.size * powl(scaler, i);
			render_frame(ft, priv);
			if (cancel_requested) break;
		}
		if (cancel_requested) break;
		for (unsigned i=0; i<iter->hold_frames; i++) {
			render_frame(f2, priv);
			if (cancel_requested) break;
		}

		traverse = iter->frames_to_next;
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
		const struct Movie::MovieInfo& movie;
		unsigned fileno;

		Private(const struct Movie::MovieInfo& _movie, const std::string& filename) : movie(_movie), fileno(0) {
			fs.open(filename, std::fstream::out);
		}
		~Private() {
			fs.close();
		}
	};
	public:
		ScriptB2CLI() : Movie::Renderer("Script for brot2cli", "*.sh") { }

		void render_top(std::shared_ptr<const BrotPrefs::Prefs>, ThreadPool&, const std::string& filename, const struct Movie::MovieInfo& movie, Movie::RenderInstancePrivate **priv) {
			std::string b2cli(brot2_argv0);
			b2cli.append("cli");
			std::string outdir(filename);
			int spos = outdir.rfind('/');
			if (spos >= 0)
				outdir.erase(spos+1);
			Private *mypriv = new Private(movie, filename);
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
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv) {
			Private * mypriv = (Private*)(priv);
			mypriv->fs
				<< "${BROT2CLI} -X " << fr.centre.real() << " -Y " << fr.centre.imag()
				<< " -l " << fr.size.real()
				<< " -f \"" << mypriv->movie.fractal->name << "\""
				<< " -p \"" << mypriv->movie.palette->name << "\""
				<< " -h " << mypriv->movie.height
				<< " -w " << mypriv->movie.width
				<< " -o \"${OUTDIR}/${OUTNAME}." << setfill('0') << setw(6) << (mypriv->fileno++) << ".png\""
				;
			if (mypriv->movie.draw_hud)
				mypriv->fs << " -H";
			if (mypriv->movie.antialias)
				mypriv->fs << " -a";
			mypriv->fs << endl;
		}
		void render_tail(Movie::RenderInstancePrivate *priv, bool) {
			Private * mypriv = (Private*)(priv);
			delete mypriv;
		}
		virtual ~ScriptB2CLI() {}
};

// ------------------------------------------------------------------------------
// Outputs a bunch of PNGs in a single directory

// TODO Allow cancellation (async - could be tricky!) - set flag from prog window, check in render()?

class BunchOfPNGs : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class BunchOfPNGs;
		const struct Movie::MovieInfo& movie;
		unsigned fileno;
		const std::string outdir, nametmpl;
		std::shared_ptr<const BrotPrefs::Prefs> prefs;
		ThreadPool& threads;
		Movie::Progress reporter;

		Private(BunchOfPNGs& renderer,
				const struct Movie::MovieInfo& _movie, const std::string& _outdir, const std::string& _tmpl,
				std::shared_ptr<const BrotPrefs::Prefs> _prefs, ThreadPool& _threads) :
			movie(_movie), fileno(0), outdir(_outdir), nametmpl(_tmpl), prefs(_prefs), threads(_threads),
			reporter(movie, renderer){
		}
		virtual ~Private() {}
	};
	public:
		BunchOfPNGs() : Movie::Renderer("PNG files in a directory", "*.png") { }

		void render_top(std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const std::string& filename, const struct Movie::MovieInfo& movie, Movie::RenderInstancePrivate **priv) {
			std::string outdir(filename), tmpl(filename);
			int spos = outdir.rfind('/');
			if (spos >= 0) {
				outdir.erase(spos+1);
				tmpl.erase(0, spos+1);
			}
			spos = tmpl.rfind(".png");
			if (spos >= 0)
				tmpl.erase(spos);
			Private *mypriv = new Private(*this, movie, outdir, tmpl, prefs, threads);
			*priv = mypriv;
		}
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv) {
			Private * mypriv = (Private*)(priv);
			ostringstream filename;
			filename << mypriv->outdir << '/' << mypriv->nametmpl << '_' << setfill('0') << setw(6) << (mypriv->fileno++) << ".png";
			std::string filename2 (filename.str());

			SavePNG::MovieFrame saver(mypriv->prefs, mypriv->threads,
					*mypriv->movie.fractal, *mypriv->movie.palette,
					mypriv->reporter,
					fr.centre, fr.size,
					mypriv->movie.width, mypriv->movie.height,
					mypriv->movie.antialias, mypriv->movie.draw_hud,
					filename2);
			saver.start();
			mypriv->reporter.set_chunks_count(saver.get_chunks_count());
			saver.wait();
			saver.save_png(0);
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
static ScriptB2CLI scripter;
static BunchOfPNGs pngz;

#define FACTORY(clazz, name, glob) \
	class clazz##Factory : public Movie::RendererFactory {                \
		public:                                                           \
				clazz##Factory() : Movie::RendererFactory(name, glob) { } \
			virtual clazz* instantiate() { return new clazz(); }          \
			virtual ~clazz##Factory() {}                                  \
	};                                                                    \
	static clazz##Factory clazz##_factory;

FACTORY(ScriptB2CLI, "Script for brot2cli", "*.sh");
FACTORY(BunchOfPNGs, "PNG files in a directory", "*.png");
