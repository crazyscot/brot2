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
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

class MainWindow;
class MovieWindowPrivate;
namespace Gtk {
	class Table;
}

class MovieWindow: public Gtk::Window{
	protected:
		MainWindow& mw;
		std::shared_ptr<const BrotPrefs::Prefs> _prefs; // master
		struct Movie::MovieInfo movie;
		MovieWindowPrivate *priv;

	public:
		MovieWindow(MainWindow& _mw, std::shared_ptr<const BrotPrefs::Prefs> prefs);
		~MovieWindow();

		bool close();
		void reset();
		bool on_delete_event(GdkEventAny *evt);

		void do_add();
		void do_delete();
		void do_reset();
		void do_render();
		void do_update_duration();
		void do_update_duration2(const Gtk::TreeModel::Path&, const Gtk::TreeModel::iterator&);
};

#endif // MOVIEWINDOW_H
