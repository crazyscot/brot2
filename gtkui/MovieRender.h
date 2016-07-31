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
#include <vector>

class MovieWindow;

namespace Movie {

class Renderer;
class RenderInstancePrivate {}; // Used by Renderer to store private data

class RenderJob; // private to MovieRender.cpp

class Renderer {
	friend class RenderJob;

	protected:
		Renderer(const std::string& name, const std::string& pattern);
		bool cancel_requested;

	public:
		const std::string name;
		const std::string pattern; // shell style glob, for Gtk::FileFilter

		// Main entrypoint:
		void start(MovieWindow& parent, const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads);

		// Initialise render run, alloc Private if needed
		virtual void render_top(std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads, const std::string& filename, const struct Movie::MovieInfo& movie, Movie::RenderInstancePrivate** priv) = 0;

		// Called for each frame
		virtual void render_frame(const struct Movie::Frame& kf, Movie::RenderInstancePrivate *priv) = 0;

		// Finish up, flush file, delete Private
		virtual void render_tail(Movie::RenderInstancePrivate *priv, bool cancelling=false) = 0;

		virtual void request_cancel();

		virtual ~Renderer();

		static SimpleRegistry<Renderer> all_renderers;

	private:
		void render(const std::string& filename, const struct Movie::MovieInfo& movie, std::shared_ptr<const BrotPrefs::Prefs> prefs, ThreadPool& threads);

};

class RendererFactory {
	friend class Renderer;
	static SimpleRegistry<RendererFactory> all_factories;
	const std::string name;
	const std::string pattern; // shell style glob, for Gtk::FileFilter

	protected:
		RendererFactory(const std::string& _name, const std::string& _pattern);
		virtual ~RendererFactory();

	public:
		static RendererFactory* get_factory(const std::string& name);
		virtual Renderer* instantiate() = 0;
};

}; // namespace Movie

#endif // MOVIERENDER_H
