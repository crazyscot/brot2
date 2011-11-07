/*
    PrefsDialog: brot2 preferences dialog
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
#include "misc.h"
#include "Plot2.h"
#include "Prefs.h"

#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/enums.h>

#include <sstream>

PrefsDialog::PrefsDialog(MainWindow *_mw) : Gtk::Dialog("Preferences", _mw, true),
	mw(_mw)
{
	add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_ACCEPT);

	// This might conceivably become a multi-pane job later.
	Gtk::Box* box = get_vbox();
	Gtk::Label *lbl = Gtk::manage(new Gtk::Label("Foo!"));
	box->pack_start(*lbl);
}

int PrefsDialog::run() {
	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();

		if (result == GTK_RESPONSE_ACCEPT) {
			Prefs& p = Prefs::getDefaultInstance();
			// Update p.
			// Perform any sanity checks here.

			if (error) {
				Util::alert(mw, "Sorry, I could not parse that; care to try again?");
			} else {
				p.commit();
				// XXX: Poke things that might need to reread.
			}
		}
	} while (error && result == GTK_RESPONSE_ACCEPT);

	return result;
}
