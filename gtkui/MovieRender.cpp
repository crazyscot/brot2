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
#include "misc.h"
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

Movie::RenderInstancePrivate::RenderInstancePrivate(const struct Movie::MovieInfo& _movie, Movie::Renderer& _renderer) : movie(_movie)
{
	//gdk_threads_enter(); // NO, because Progress::Progress calls threads_enter()
	reporter = new Movie::Progress(_movie, _renderer);
	//gdk_threads_leave();
}
Movie::RenderInstancePrivate::~RenderInstancePrivate()
{
	gdk_threads_enter();
	delete reporter; // It's a Gtk::Window
	gdk_threads_leave();
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
			_parent.signal_completion(_renderer);
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

	// NOTE: The total number of frames we render should match the number calculated by MovieWindow::do_update_duration().

	struct Movie::Frame f1;
	f1.centre.real(iter->centre.real());
	f1.centre.imag(iter->centre.imag());
	f1.size.real(  iter->size.real());
	f1.size.imag(  iter->size.imag());
	render_frame(f1, priv, iter->hold_frames); // we expect this will call plot_complete but not frames_traversed
	priv->reporter->frames_traversed(iter->hold_frames);
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
			render_frame(ft, priv, 1);
			if (cancel_requested) break;
		}
		if (cancel_requested) break;
		render_frame(f2, priv, iter->hold_frames); // we expect this will call plot_complete but not frames_traversed
		priv->reporter->frames_traversed(iter->hold_frames);

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
		unsigned fileno;

		Private(ScriptB2CLI& _renderer, const struct Movie::MovieInfo& _movie, const std::string& filename) : Movie::RenderInstancePrivate(_movie, _renderer), fileno(0) {
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
			Private *mypriv = new Private(*this, movie, filename);
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
				<< " -f \"" << mypriv->movie.fractal->name << "\""
				<< " -p \"" << mypriv->movie.palette->name << "\""
				<< " -h " << mypriv->movie.height
				<< " -w " << mypriv->movie.width
				<< " -o " << filename.str()
				;
			if (mypriv->movie.draw_hud)
				mypriv->fs << " -H";
			if (mypriv->movie.antialias)
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

// TODO Allow cancellation (async - could be tricky!) - set flag from prog window, check in render()?

class BunchOfPNGs : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class BunchOfPNGs;
		unsigned fileno;
		const std::string outdir, nametmpl;
		std::shared_ptr<const BrotPrefs::Prefs> prefs;
		ThreadPool& threads;

		Private(BunchOfPNGs& renderer,
				const struct Movie::MovieInfo& _movie, const std::string& _outdir, const std::string& _tmpl,
				std::shared_ptr<const BrotPrefs::Prefs> _prefs, ThreadPool& _threads) :
			RenderInstancePrivate(_movie, renderer), fileno(0), outdir(_outdir), nametmpl(_tmpl), prefs(_prefs), threads(_threads)
		{
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
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv, unsigned n_frames) {
			Private * mypriv = (Private*)(priv);
			ostringstream filename;
			filename << mypriv->outdir << '/' << mypriv->nametmpl << '_' << setfill('0') << setw(6) << (mypriv->fileno++) << ".png";
			std::string filename2 (filename.str());

			SavePNG::MovieFrame saver(mypriv->prefs, mypriv->threads,
					*mypriv->movie.fractal, *mypriv->movie.palette,
					*mypriv->reporter,
					fr.centre, fr.size,
					mypriv->movie.width, mypriv->movie.height,
					mypriv->movie.antialias, mypriv->movie.draw_hud,
					filename2);
			saver.start();
			mypriv->reporter->set_chunks_count(saver.get_chunks_count());
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

#define FACTORY(clazz, name, glob) \
	class clazz##Factory : public Movie::RendererFactory {                \
		public:                                                           \
				clazz##Factory() : Movie::RendererFactory(name, glob) { } \
			virtual std::shared_ptr<Movie::Renderer> instantiate() {      \
				std::shared_ptr<Movie::Renderer> instance(new clazz());   \
				return instance;                                          \
			}                                                             \
			virtual ~clazz##Factory() {}                                  \
	};                                                                    \
	static clazz##Factory clazz##_factory;

FACTORY(ScriptB2CLI, "Script for brot2cli", "*.sh");
FACTORY(BunchOfPNGs, "PNG files in a directory", "*.png");
