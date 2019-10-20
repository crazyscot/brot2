/*
    MovieWindow: brot2 movie creator window
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

#ifndef MOVIEWINDOW_H
#define MOVIEWINDOW_H

#include "Prefs.h"
#include "MovieMode.h"
#include "MovieRender.h"
#include "misc.h"
BROT2_GLIBMM_BEFORE
BROT2_GTKMM_BEFORE
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/cellrenderer.h>
#include <gtkmm/treeview.h>
#include <condition_variable>
BROT2_GTKMM_AFTER
BROT2_GLIBMM_AFTER

class MainWindow;
class MovieWindowPrivate;
namespace Gtk {
	class Table;
}
namespace Movie {
	class Renderer;
	class RenderJob;
};

class MovieWindow: public Gtk::Window, public Movie::IMovieCompleteHandler {
	protected:
		MainWindow& mw;
		std::shared_ptr<const BrotPrefs::Prefs> _prefs; // master
		struct Movie::MovieInfo movie;
		MovieWindowPrivate *priv;
		std::shared_ptr<Movie::Renderer> renderer; // 0 when inactive, protected by mux

		std::mutex mux; // Protects renderer and completion_cv
		std::condition_variable completion_cv; // Signals to anyone who's interested that we're complete

		void treeview_fractal_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter, int model_column);
		int treeview_append_fractal_column(Gtk::TreeView& it, const Glib::ustring& title, const Gtk::TreeModelColumn<Fractal::Value>& model_column);
		void treeview_integer_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter, int model_column, unsigned min, unsigned max);
		int treeview_append_editable_integer_column(Gtk::TreeView& it, const Glib::ustring& title, const Gtk::TreeModelColumn<unsigned>& model_column, unsigned min, unsigned max);


	public:
		MovieWindow(MainWindow& _mw, std::shared_ptr<const BrotPrefs::Prefs> prefs);
		~MovieWindow();

		void initial_position(); // Repositions the window to its intended initial position, if this is the first time it has been opened.

		bool on_delete_event(GdkEventAny *evt);

		void do_add();
		void do_delete();
		void do_reset();
		void do_render();
		void do_save();
		void do_load();
		void do_update_duration();
		void do_update_duration2(const Gtk::TreeModel::Path&, const Gtk::TreeModel::iterator&);
		void do_show();

		bool update_movie_struct(); // Populates @movie@ from the Gtk tree; returns true on success. On unparseable fields, alerts and returns false.
		void update_from_movieinfo(const struct Movie::MovieInfo& new1); // Updates the window from the struct contents (if we've just loaded). The opposite of update_movie_struct().
		void update_fractal_palette_display();

		bool run_filename(std::string& filename, std::shared_ptr<Movie::Renderer>& ren);
		void signal_completion(Movie::RenderJob& job); // IMovieCompleteHandler
		void signal_error(Movie::RenderJob& job, const std::string& msg); // IMovieCompleteHandler

		void stop(); // Attempts to halt the render ASAP
		void wait(); // Waits for all movie jobs to complete
};

#endif // MOVIEWINDOW_H
