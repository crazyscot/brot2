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
#include "Prefs.h"

#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

#include <sstream>

// TODO Make this non-modal!

namespace Actions {

class Columns : public Gtk::TreeModel::ColumnRecord
{
public:
	Gtk::TreeModelColumn<Glib::ustring> name;
	Gtk::TreeModelColumn<int> val;

	Columns() {
		add(name); add(val);
	}
};

class Combo : public Gtk::ComboBox {
public:
	Combo() : Gtk::ComboBox() {
		init_master_model();
		set_model(master_model);
		set_entry_text_column(1);
		pack_start(cols.name); // column(s) to display in order.
	}

protected:
	static Columns cols;
	static Glib::RefPtr< Gtk::ListStore > master_model;
	static bool model_inited;
	static void init_master_model() {
		if (!model_inited) {
			master_model = Gtk::ListStore::create(cols);
			Gtk::TreeIter iter;

#define POPULATE(_NAME,_VAL) do {				\
			iter = master_model->append(); 		\
			Glib::ustring tmp = #_NAME;			\
			(*iter)->set_value(cols.name, tmp);	\
			(*iter)->set_value(cols.val, _VAL);	\
} while(0);
			ALL_ACTIONS(POPULATE);

			model_inited = true;
		}
	}

	// XXX do we need an on_changed to stash the active enu?
};

Glib::RefPtr< Gtk::ListStore > Combo::master_model;
bool Combo::model_inited;
Columns Combo::cols;

}; // namespace Actions

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

	Actions::Combo* combo = Gtk::manage(new Actions::Combo());
	tbl->attach(*combo, 0, 1, 1, 2);

	// XXX: Keep pointers to the combos somewhere.
	// Do this by defining a panel class (in this cpp) ...

	box->pack_start(*tbl);
}

int PrefsDialog::run() {
	// XXX set up widgets/panel from current prefs
	// combo.set_active_row_number()

	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();
		Fractal::Point new_ctr, new_size;

		if (result == GTK_RESPONSE_ACCEPT) {
			// XXX read out widgets/panel
			// combo.get_active() returns an Iterator - then read its val.

			if (error) {
				Util::alert(mw, "Sorry, I could not parse that; care to try again?");
			} else {
				// XXX update prefs/etc via mw.
			}
		}
	} while (error && result == GTK_RESPONSE_ACCEPT);

	return result;
}
