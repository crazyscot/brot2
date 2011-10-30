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
#include <gtkmm/frame.h>
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

	void set(Action a) {
		assert(model_inited);
		Gtk::TreeModel::Children entries = master_model->children();
		Gtk::TreeModel::iterator it = entries.begin();
		for ( ; it != entries.end(); it++) {
			if ((*it)->get_value(cols.val) == (int)a) {
				set_active(it);
				break;
			}
		}
	}

	int get() const {
		const Gtk::TreeModel::iterator it = get_active();
		return (*it)->get_value(cols.val);
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
};

Glib::RefPtr< Gtk::ListStore > Combo::master_model;
bool Combo::model_inited;
Columns Combo::cols;

class MouseButtonsPanel {
	private:
		Combo* actions[MouseActions::MAX+1];

	public:
		MouseButtonsPanel(const Prefs& prefs) {
			const MouseActions& ma = prefs.mouseActions();
			for (int i=MouseActions::MIN; i<=MouseActions::MAX; i++) {
				actions[i] = Gtk::manage(new Combo());
				actions[i]->set(ma[i]);
			}
		}

		void saveToPrefs(Prefs &prefs) {
			MouseActions ma;
			for (int i=MouseActions::MIN; i<=MouseActions::MAX; i++) {
				ma[i] = actions[i]->get();
			}
			prefs.mouseActions(ma);
		}

		Gtk::Frame *frame() {
			Gtk::Frame *frm = Gtk::manage(new Gtk::Frame("Mouse button actions"));

			Gtk::Table *tbl = Gtk::manage(new Gtk::Table(MouseActions::MAX, 2, false));
			for (int i=MouseActions::MIN; i<=MouseActions::MAX; i++) {
				char buf[32];
				Gtk::Label* label;
				switch (i) {
					case 1:
						snprintf(buf, sizeof buf, "Left");
						break;
					case 2:
						snprintf(buf, sizeof buf, "Middle");
						break;
					case 3:
						snprintf(buf, sizeof buf, "Right");
						break;
					default:
						snprintf(buf, sizeof buf, "Button %d", i);
						break;
				}
				Glib::ustring txt(buf);
				label = Gtk::manage(new Gtk::Label(txt));
				label->set_alignment(0, 0.5);

				tbl->attach(*label, 0, 1, i, i+1);
				tbl->attach(*actions[i], 1, 2, i, i+1);
			}
			frm->add(*tbl);

			return frm;
		}
};

}; // namespace Actions

PrefsDialog::PrefsDialog(MainWindow *_mw) : Gtk::Dialog("Preferences", _mw, true),
	mw(_mw)
{
	add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_ACCEPT);

	Gtk::Box* box = get_vbox();
	Gtk::Table *tbl = Gtk::manage(new Gtk::Table(3,1,false)); // rows,cols

    mouse = new Actions::MouseButtonsPanel(Prefs::getDefaultInstance());
	tbl->attach(*mouse->frame(), 0, 1, 0, 1);

	box->pack_start(*tbl);
}

PrefsDialog::~PrefsDialog() {
	delete mouse;
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
			mouse->saveToPrefs(p);
			// Perform any sanity checks here.

			if (error) {
				Util::alert(mw, "Sorry, I could not parse that; care to try again?");
			} else {
				p.commit();
			}
		}
	} while (error && result == GTK_RESPONSE_ACCEPT);

	return result;
}
