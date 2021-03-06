/*
    ControlsWindow: brot2 mouse/scroll configuration dialog
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

#include "png.h" // see launchpad 218409
#include "ControlsWindow.h"
#include "MainWindow.h"
#include "Fractal.h"
#include "misc.h"
#include "gtkutil.h"
#include "Prefs.h"
#include "Exception.h"

#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/enums.h>
#include <gtkmm/alignment.h>

#include <sstream>

using namespace BrotPrefs;

namespace Actions {

class Columns : public Gtk::TreeModel::ColumnRecord
{
public:
	Gtk::TreeModelColumn<Glib::ustring> name;
	Gtk::TreeModelColumn<int> val;
	Gtk::TreeModelColumn<Glib::ustring> text;

	Columns() {
		add(name); add(val); add(text);
	}
};

class NotifyTarget {
	public:
		virtual void notify() = 0;
	protected:
		~NotifyTarget() {}
};

class Combo : public Gtk::ComboBox {
public:
	Combo(NotifyTarget& _parent) : Gtk::ComboBox(), parent(_parent) {
		init_master_model();
		set_model(master_model);
		set_entry_text_column(1);
		pack_start(cols.text); // column(s) to display in order.

		signal_changed().connect(sigc::mem_fun(*this, &Combo::on_changed) );
	}

	void set(Action a) {
		ASSERT(model_inited);
		Gtk::TreeModel::Children entries = master_model->children();
		Gtk::TreeModel::iterator it = entries.begin();
		for ( ; it != entries.end(); it++) {
			if ((*it)->get_value(cols.val) == (int)a) {
				set_active(it);
				break;
			}
		}
	}

	void on_changed() {
		parent.notify();
	}

	int get() const {
		const Gtk::TreeModel::iterator it = get_active();
		return (*it)->get_value(cols.val);
	}


protected:
	static Columns cols;
	NotifyTarget &parent;
	static Glib::RefPtr< Gtk::ListStore > master_model;
	static bool model_inited;
	static void init_master_model() {
		if (!model_inited) {
			master_model = Gtk::ListStore::create(cols);
			Gtk::TreeIter iter;

#define POPULATE(_NAME,_VAL,_TEXT) do {			\
			iter = master_model->append(); 		\
			Glib::ustring tmp = #_NAME;			\
			(*iter)->set_value(cols.name, tmp);	\
			(*iter)->set_value(cols.val, _VAL);	\
			tmp = #_TEXT;						\
			(*iter)->set_value(cols.text, tmp);	\
} while(0);
			ALL_ACTIONS(POPULATE);

			model_inited = true;
		}
	}
};

Glib::RefPtr< Gtk::ListStore > Combo::master_model;
bool Combo::model_inited;
Columns Combo::cols;

class MouseButtonsPanel : NotifyTarget {
	private:
		Combo* actions[MouseActions::MAX+1];
		std::shared_ptr<const Prefs> _prefs;
		bool initialising;

	public:
		MouseButtonsPanel(std::shared_ptr<const Prefs> prefs) : _prefs(prefs) {
			initialising = true;
			for (int i=MouseActions::MIN; i<=MouseActions::MAX; i++) {
				actions[i] = Gtk::manage(new Combo(*this));
			}
			refresh(); // itself sets/clears initialising
			initialising = false;
		}

		virtual ~MouseButtonsPanel() {}

		void saveToPrefs() {
			MouseActions ma;
			for (int i=MouseActions::MIN; i<=MouseActions::MAX; i++) {
				ma[i] = actions[i]->get();
			}
			std::shared_ptr<Prefs> tmpprefs = _prefs->getWorkingCopy();
			tmpprefs->mouseActions(ma);
			tmpprefs->commit();
		}

		virtual void notify() {
			if (!initialising)
				saveToPrefs();
		}

		void defaults() {
			std::shared_ptr<Prefs> tmpprefs = _prefs->getWorkingCopy();
			MouseActions ma(_prefs->mouseActions());
			ma.set_to_default();
			tmpprefs->mouseActions(ma);
			tmpprefs->commit();
			refresh();
		}

	protected:
		void refresh(void) {
			initialising = true;
			const MouseActions& ma = _prefs->mouseActions();
			for (int i=MouseActions::MIN; i<=MouseActions::MAX; i++) {
				actions[i]->set(ma[i]);
			}
			initialising = false;
		}

	public:
		Gtk::Frame *frame() {
			Gtk::Frame *frm = Gtk::manage(new Gtk::Frame("Mouse button actions"));
			frm->set_border_width(10);

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

class ScrollButtonsPanel : public NotifyTarget {
	private:
		Combo* actions[ScrollActions::MAX+1];
		std::shared_ptr<const Prefs> _prefs;
		bool initialising;

	public:
		ScrollButtonsPanel(std::shared_ptr<const Prefs> prefs) : _prefs(prefs) {
			initialising = true;
			for (int i=ScrollActions::MIN; i<=ScrollActions::MAX; i++) {
				actions[i] = Gtk::manage(new Combo(*this));
			}
			refresh(); // itself sets/clears initialising
			initialising = false;
		}

		virtual ~ScrollButtonsPanel() {}

		void saveToPrefs() {
			std::shared_ptr<Prefs> tmpprefs = _prefs->getWorkingCopy();
			ScrollActions ma;
			for (int i=ScrollActions::MIN; i<=ScrollActions::MAX; i++) {
				ma[i] = actions[i]->get();
			}
			tmpprefs->scrollActions(ma);
			tmpprefs->commit();
		}

		virtual void notify() {
			if (!initialising)
				saveToPrefs();
		}

		void defaults() {
			std::shared_ptr<Prefs> tmpprefs = _prefs->getWorkingCopy();
			ScrollActions sa(_prefs->scrollActions());
			sa.set_to_default();
			tmpprefs->scrollActions(sa);
			tmpprefs->commit();
			refresh();
		}

	protected:
		void refresh(void) {
			initialising = true;
			const ScrollActions& sa = _prefs->scrollActions();
			for (int i=ScrollActions::MIN; i<=ScrollActions::MAX; i++) {
				actions[i]->set(sa[i]);
			}
			initialising = false;
		}

	public:
		Gtk::Frame *frame() {
			Gtk::Frame *frm = Gtk::manage(new Gtk::Frame("Scroll wheel actions"));
			frm->set_border_width(10);

			Gtk::Table *tbl = Gtk::manage(new Gtk::Table(ScrollActions::MAX, 2, false));
			for (int i=ScrollActions::MIN; i<=ScrollActions::MAX; i++) {
				char buf[32];
				Gtk::Label* label;
				switch (i) {
					case GDK_SCROLL_UP:
						snprintf(buf, sizeof buf, "Up");
						break;
					case GDK_SCROLL_DOWN:
						snprintf(buf, sizeof buf, "Down");
						break;
					case GDK_SCROLL_LEFT:
						snprintf(buf, sizeof buf, "Left");
						break;
					case GDK_SCROLL_RIGHT:
						snprintf(buf, sizeof buf, "Right");
						break;
					default:
						snprintf(buf, sizeof buf, "Scroll direction %d", i);
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

ControlsWindow::ControlsWindow(MainWindow& _mw, std::shared_ptr<const Prefs> prefs) : mw(_mw), _prefs(prefs)
{
	set_title("Controls");

	Gtk::VBox* box = Gtk::manage(new Gtk::VBox());
	box->set_border_width(1);
	add(*box);

	Gtk::Table *tbl = Gtk::manage(new Gtk::Table(3,3,false)); // rows,cols

    mouse = new Actions::MouseButtonsPanel(_prefs);
	tbl->attach(*mouse->frame(), 0, 1, 0, 2); // left right top bottom

	// scroll is shorter than mouse, don't let it expand (looks silly)
	Gtk::Alignment *align = Gtk::manage(new Gtk::Alignment(0.5, 0.0, 0.0, 0.0));
    scroll = new Actions::ScrollButtonsPanel(_prefs);
	align->add(*scroll->frame());
	tbl->attach(*align, 1, 2, 0, 1);

	box->pack_start(*tbl);

	// And now a separate table for the Defaults button.
	Gtk::HButtonBox *bbox = Gtk::manage(new Gtk::HButtonBox());
	Gtk::Button *btn = Gtk::manage(new Gtk::Button("Defaults"));
	bbox->add(*btn);
	box->pack_end(*bbox);
	btn->signal_clicked().connect(sigc::mem_fun(*this, &ControlsWindow::on_defaults) );

	hide();
	box->show_all();
}

void ControlsWindow::starting_position() {
	/* This window will attempt to appear in line with the main window, offset to the right.
	 * If this is not possible it will appear at an edge. */
	int ww, hh, mx, my;
	Util::get_screen_geometry(*this, ww, hh);
	mw.get_position(mx, my);
	int xx = mx + mw.get_width();
	int yy = my;
	if (xx+get_width() > ww) xx = mx-get_width(); // Off screen? Try the left.
	Util::fix_window_coords(*this, xx, yy); // No? (test case: Main is fullscreen) Just put it flush left.
	move(xx,yy);
}

void ControlsWindow::on_defaults() {
	mouse->defaults();
	scroll->defaults();
}

ControlsWindow::~ControlsWindow() {
	delete mouse;
	delete scroll;
}

bool ControlsWindow::close() {
	hide();
	mw.optionsMenu()->set_controls_status(false);
	return true;
}

bool ControlsWindow::on_delete_event(GdkEventAny *evt) {
	(void)evt;
	return close();
}
