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
#include "Prefs.h"
#include "Fractal.h"
#include "FractalMaths.h"
#include "palette.h"
#include "Registry.h"
#include "ThreadPool.h"
#include "IPlot3DataSink.h"
#include <vector>
#include <set>
#include <atomic>

namespace Movie {

class Renderer;
class IRenderCompleteHandler {
	public:
		// Signals that the given movie job has been completed.
		virtual void signal_completion(Renderer& job) = 0;
		virtual ~IRenderCompleteHandler() {}
};

class IRenderProgressReporter : public Plot3::IPlot3DataSink {
	public:
		// Setup. Not mandatory, we do the best we can if not called.
		virtual void set_chunks_count(int n) = 0;
		// Call when outputting multiple frames at once. We record @n@ frames as being plotted.
		virtual void frames_traversed(int n) = 0;

		// We also inherit from  Plot3::IPlot3DataSink:
		// virtual void chunk_done(Plot3::Plot3Chunk* job) = 0;
		// virtual void pass_complete(std::string& msg, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels) = 0;
		// virtual void plot_complete() = 0; // One plot = one FRAME of the movie.

		virtual ~IRenderProgressReporter() {}
};

struct RenderInstancePrivate {
	const struct Movie::MovieInfo& movie;
	IRenderProgressReporter *reporter;
	RenderInstancePrivate(const struct Movie::MovieInfo& _movie, Movie::Renderer& _renderer);
	virtual ~RenderInstancePrivate();
}; // Used by Renderer to store private data

class RenderJob; // private to MovieRender.cpp

class Renderer {
	friend class RenderJob;

	protected:
		Renderer(const std::string& name, const std::string& pattern);
		std::atomic<bool> cancel_requested;

	public:
		const std::string name;
		const std::string pattern; // shell style glob, for Gtk::FileFilter

		// Main entrypoint:
		void start(IRenderCompleteHandler& completion, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads);

		// Initialise render run, alloc Private if needed
		virtual void render_top(std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const std::string& filename, const struct Movie::MovieInfo& movie, Movie::RenderInstancePrivate** priv) = 0;

		// Called for each frame in turn; "n_frames" is the number of times this frame is to be inserted
		virtual void render_frame(const struct Movie::Frame& kf, Movie::RenderInstancePrivate *priv, const unsigned n_frames=1) = 0;

		// Finish up, flush file, delete Private
		virtual void render_tail(Movie::RenderInstancePrivate *priv, bool cancelling=false) = 0;

		virtual void request_cancel();

		virtual ~Renderer();

	private:
		void render(const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads);

};

class RendererFactory {
	friend class Renderer;
	static SimpleRegistry<RendererFactory> all_factories;

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

}; // namespace Movie

#endif // MOVIERENDER_H
