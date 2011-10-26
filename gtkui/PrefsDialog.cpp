/*
    PrefsDialog: brot2 fractal parameters configuration dialog
    Copyright (C) 2011 Ross Younger

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

#include "PrefsDialog.h"
#include "MainWindow.h"
#include "Fractal.h"
#include "misc.h"
#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>

#include <sstream>

// TODO Make this non-modal!

PrefsDialog::PrefsDialog(MainWindow *_mw) : Gtk::Dialog("Preferences", _mw, true),
	mw(_mw)
{
	add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_ACCEPT);

	Gtk::Box* box = get_vbox();
	Gtk::Table *tbl = Gtk::manage(new Gtk::Table(3,2));

	Gtk::Label* label;

	label = Gtk::manage(new Gtk::Label("foobarbazqux"));
	label->set_alignment(0.5, 0.5);
	tbl->attach(*label, 0, 1, 0, 1);

	box->pack_start(*tbl);
}

int PrefsDialog::run() {
	// XXX set up widgets from current prefs

	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();
		Fractal::Point new_ctr, new_size;

		if (result == GTK_RESPONSE_ACCEPT) {
			// XXX read out widgets

			if (error) {
				Util::alert(mw, "Sorry, I could not parse that; care to try again?");
			} else {
				// XXX update prefs via mw.
			}
		}
	} while (error && result == GTK_RESPONSE_ACCEPT);

	return result;
}
