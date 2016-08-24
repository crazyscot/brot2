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
#include "Exception.h"
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
	std::shared_ptr<ThreadPool> _threads;
	const char *_argv0; // The CLI used to invoke brot2. This is used in at least one renderer to locate brot2cli.
	const unsigned _rwidth, _rheight; // Rendering width and height. Will be larger than movie dimensions if antialias enabled.

	// RenderJob takes a ThreadPool for its worker threads. A separate thread will be spawned to wait on the render itself.
	RenderJob(IMovieProgressReporter& reporter, IMovieCompleteHandler& parent, Movie::Renderer& renderer, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> worker_threads, const char* argv0);
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
		void start(IMovieProgressReporter& reporter, IMovieCompleteHandler& completion, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> threads, const char* argv0);
		// Special entrypoint for unthreaded mode:
		void do_blocking(IMovieProgressReporter& reporter, IMovieCompleteHandler& completion, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> threads, const char* argv0);

		// Initialise render run, alloc Private (mandatory; must not be left null)
		virtual void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate** priv) = 0;

		// Called for each frame in turn; "n_frames" is the number of times this frame is to be inserted.
		// Should call the job's reporter's plot_complete() precisely once.
		// Should NOT call frames_traversed when n_frames>1; that is done for you by Renderer::render().
		virtual void render_frame(const struct Movie::Frame& kf, Movie::RenderInstancePrivate *priv, const unsigned n_frames=1) = 0;

		// Finish up, flush file, do NOT delete Private as the framework does that for you.
		virtual void render_tail(Movie::RenderInstancePrivate *priv, bool cancelling=false) = 0;

		virtual void request_cancel();

		virtual ~Renderer();

	private:
		// Main rendering loop, calls render_frame() repeatedly
		void render(Movie::RenderInstancePrivate *priv);

		static ThreadPool movie_runner_thread;
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

// This macro creates a factory for a class, allowing its details to be kept private to the source compile unit.
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


// -----------------------------------------------------------------
// The null renderer is special, used for counting frames before actually rendering.
// It doesn't have a factory so that it doesn't appear in the list of options.
// Just instantiate it as a standard class.

const unsigned FRAME_LIMIT=1000000; // At 25fps, this is about 11 hours. More than enough!

// Exception thrown internally, will not be seen outside of MovieInfo::count_frames().
struct FrameLimitExceeded : public BrotException {
	FrameLimitExceeded() : BrotException("Frame limit exceeded") {}
};

class NullRenderer : public Movie::Renderer {
	public:
		unsigned framecount_;
		NullRenderer() : Movie::Renderer("Null renderer", "/dev/null"), framecount_(0) { }

		void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate **priv) {
			*priv = new Movie::RenderInstancePrivate(job);
			framecount_ = 0;
		}
		void render_frame(const struct Movie::Frame&, Movie::RenderInstancePrivate *, const unsigned n_frames) {
			framecount_ += n_frames;
			if (framecount_ > Movie::FRAME_LIMIT)
				throw FrameLimitExceeded();
		}
		void render_tail(Movie::RenderInstancePrivate*, bool) { }
		virtual ~NullRenderer() {}

		virtual unsigned framecount() { return framecount_; }
};

}; // namespace Movie

// Standardised names for renderers are listed here, where it is necessary to refer to them outside of their own compile unit.
#define MOVIERENDER_NAME_MOV "QuickTime MOV/H264"

#endif // MOVIERENDER_H
