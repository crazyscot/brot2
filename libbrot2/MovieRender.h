/*
    MovieMode: brot2 movie plotting
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

#ifndef MOVIERENDER_H
#define MOVIERENDER_H

#include "MovieMode.h"
#include "IMovieProgress.h"
#include "Prefs.h"
#include "Fractal.h"
#include "FractalMaths.h"
#include "palette.h"
#include "Registry.h"
#include "ThreadPool.h"
#include <vector>
#include <set>
#include <atomic>

namespace Movie {

class Renderer;

struct RenderJob {
	// Everything about a job, from the UI's point of view, goes in here.
	IMovieCompleteHandler& _parent;
	IMovieProgressReporter *_reporter;
	Movie::Renderer& _renderer;
	const std::string _filename;
	const struct Movie::MovieInfo _movie;
	std::shared_ptr<const BrotPrefs::Prefs> _prefs;
	ThreadPool& _threads;
	const char *_argv0; // The CLI used to invoke brot2. This is used in at least one renderer to locate brot2cli.

	RenderJob(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, Movie::Renderer& renderer, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const char* argv0);
	void run();
	virtual ~RenderJob();
};

struct RenderInstancePrivate {
	// Subclassed by Renderer subclasses to store private data
	RenderJob& job;

	RenderInstancePrivate(Movie::RenderJob& _job);
	virtual ~RenderInstancePrivate();
};

class Renderer {
	// Renderer interface, you get instances of derived classes from the Factory
	friend struct RenderJob;

	protected:
		Renderer(const std::string& name, const std::string& pattern);
		std::atomic<bool> cancel_requested;

	public:
		const std::string name;
		const std::string pattern; // shell style glob, for Gtk::FileFilter

		// Main entrypoint:
		void start(IMovieProgressReporter& reporter, IMovieCompleteHandler& completion, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const char* argv0);

		// Initialise render run, alloc Private if needed
		virtual void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate** priv) = 0;

		// Called for each frame in turn; "n_frames" is the number of times this frame is to be inserted
		virtual void render_frame(const struct Movie::Frame& kf, Movie::RenderInstancePrivate *priv, const unsigned n_frames=1) = 0;

		// Finish up, flush file, delete Private
		virtual void render_tail(Movie::RenderInstancePrivate *priv, bool cancelling=false) = 0;

		virtual void request_cancel();

		virtual ~Renderer();

	private:
		void render(Movie::RenderJob* job);

};

class RendererFactory {
	friend class Renderer;
	static SimpleRegistry<RendererFactory>* all_factories();

	protected:
		RendererFactory(const std::string& _name, const std::string& _pattern);
		virtual ~RendererFactory();

	public:
		const std::string name;
		const std::string pattern; // shell style glob, for Gtk::FileFilter
		static std::set<std::string> all_factory_names();
		static RendererFactory* get_factory(const std::string& name);
		virtual std::shared_ptr<Renderer> instantiate() = 0;
};

class NullRendererFactory {
	public:
		std::shared_ptr<Renderer> instantiate();
};

}; // namespace Movie

#define MOVIERENDER_DECLARE_FACTORY(clazz, name, glob) \
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


#endif // MOVIERENDER_H
