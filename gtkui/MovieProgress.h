/*
    MovieProgress: brot2 movie progress window
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

#ifndef MOVIEPROGRESS_H
#define MOVIEPROGRESS_H

#include "IMovieProgress.h"
#include "MovieRender.h"
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>

namespace Movie {
	class Renderer;

	class Progress: public Gtk::Window, public Movie::IRenderProgressReporter {
		private:
			Movie::Renderer& renderer;
			Gtk::VBox* vbox;
			Gtk::ProgressBar *plotbar, *framebar, *moviebar;
			int chunks_done, chunks_count, frames_done;
			const unsigned npixels, nframes;
			const struct MovieInfo& movie;

			bool on_delete_event(GdkEventAny *evt);
			bool on_timer();
			void do_cancel();
		public:
			Progress(const struct MovieInfo &, Movie::Renderer &); // Must hold the GDK threads lock!
			virtual ~Progress();

			virtual void chunk_done(Plot3::Plot3Chunk* job);
			virtual void pass_complete(std::string& msg, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels);
			virtual void plot_complete();

			virtual void frames_traversed(int n);

			virtual void set_chunks_count(int n);
	};
}; // namespace Movie

#endif // MOVIEPROGRESS_H
