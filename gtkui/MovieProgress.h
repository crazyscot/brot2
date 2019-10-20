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
#include "misc.h"
BROT2_GLIBMM_BEFORE
BROT2_GTKMM_BEFORE
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/button.h>
BROT2_GTKMM_AFTER
BROT2_GLIBMM_AFTER

class MovieWindow;

namespace Movie {
	class Renderer;

	class Progress: public Gtk::Window, public Movie::IMovieProgressReporter {
		private:
			MovieWindow& parent;
			Movie::Renderer& renderer;
			Gtk::VBox* vbox;
			Gtk::ProgressBar *plotbar, *framebar, *moviebar;
			Gtk::Button *cancel_btn;
			int chunks_done, chunks_count, frames_done;
			const unsigned npixels, nframes;
			const struct MovieInfo& movie;

			bool on_delete_event(GdkEventAny *evt);
			bool on_timer();
			void do_cancel();
		protected:
			virtual void chunk_done_gdklocked(Plot3::Plot3Chunk* job); // Caller must have the gdk threads lock.
			void update_chunks_bar_gdklocked(); // Caller must have the gdk threads lock.
			virtual void pass_complete_gdklocked(std::string& msg, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels); // Caller must have the gdk threads lock.
			virtual void frames_traversed_gdklocked(int n); // Caller must have the gdk threads lock.

		public:
			Progress(MovieWindow& parent, const struct MovieInfo &, Movie::Renderer &); // Must hold the GDK threads lock!
			virtual ~Progress();

			// These 3 functions, from IPlot3DataSink, all lock the GDK lock.
			virtual void chunk_done(Plot3::Plot3Chunk* job);
			virtual void pass_complete(std::string& msg, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels);
			virtual void plot_complete();

			virtual void frames_traversed(int n); // Locks the gdk lock, then calls frames_traversed_gdklocked
			virtual void set_chunks_count(int n);
	};
}; // namespace Movie

#endif // MOVIEPROGRESS_H
