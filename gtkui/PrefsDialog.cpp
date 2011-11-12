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
#include "Exception.h"

#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/enums.h>
#include <gtkmm/scale.h>

#include <sstream>

namespace PrefsDialogBits {
	class ThresholdFrame : public Gtk::Frame {
		public:
		// Editable fields:
		Util::HandyEntry<int> *f_init_maxiter, *f_min_done_pct;
		Util::HandyEntry<double> *f_live_threshold;

		ThresholdFrame() : Gtk::Frame("Plot finish threshold tuning") {
			f_init_maxiter = Gtk::manage(new Util::HandyEntry<int>());
			f_init_maxiter->set_activates_default(true);
			f_min_done_pct = Gtk::manage(new Util::HandyEntry<int>());
			f_min_done_pct->set_activates_default(true);
			f_live_threshold = Gtk::manage(new Util::HandyEntry<double>());
			f_live_threshold->set_activates_default(true);

			set_border_width(10);
			Gtk::Table *tbl = Gtk::manage(new Gtk::Table(3, 2, false));
			Gtk::Label *lbl;

			lbl = Gtk::manage(new Gtk::Label(PREFNAME(InitialMaxIter)));
			lbl->set_tooltip_text(PREFDESC(InitialMaxIter));
			f_init_maxiter->set_tooltip_text(PREFDESC(InitialMaxIter));
			tbl->attach(*lbl, 0, 1, 0, 1);
			tbl->attach(*f_init_maxiter, 1, 2, 0, 1);

			lbl = Gtk::manage(new Gtk::Label(PREFNAME(MinEscapeePct)));
			lbl->set_tooltip_text(PREFDESC(MinEscapeePct));
			f_min_done_pct->set_tooltip_text(PREFDESC(MinEscapeePct));
			tbl->attach(*lbl, 0, 1, 1, 2);
			tbl->attach(*f_min_done_pct, 1, 2, 1, 2);

			lbl = Gtk::manage(new Gtk::Label(PREFNAME(LiveThreshold)));
			lbl->set_tooltip_text(PREFDESC(LiveThreshold));
			f_live_threshold->set_tooltip_text(PREFDESC(LiveThreshold));
			tbl->attach(*lbl, 0, 1, 2, 3);
			tbl->attach(*f_live_threshold, 1, 2, 2, 3);

			add(*tbl);
		}

		void prepare(const Prefs& prefs) {
			f_init_maxiter->update(prefs.get(PREF(InitialMaxIter)));
			f_min_done_pct->update(prefs.get(PREF(MinEscapeePct)));
			f_live_threshold->update(prefs.get(PREF(LiveThreshold)), 4);
		}

		void defaults() {
			f_init_maxiter->update(PREF(InitialMaxIter)._default);
			f_min_done_pct->update(PREF(MinEscapeePct)._default);
			f_live_threshold->update(PREF(LiveThreshold)._default, 4);
		}

		void readout(Prefs& prefs) throw(Exception) {
			unsigned tmpu;
			int tmpi=0;

			if (!f_init_maxiter->read(tmpi))
				THROW(Exception,"Sorry, I don't understand your initial maxiter");
			if ((tmpi < PREF(InitialMaxIter)._min) || (tmpi > PREF(InitialMaxIter)._max))
				THROW(Exception,"Initial maxiter must be at least 2");
			tmpu = tmpi;
			prefs.set(PREF(InitialMaxIter), tmpu);

			if (!f_min_done_pct->read(tmpi))
				THROW(Exception,"Sorry, I don't understand your Minimum done %");
			if ((tmpi<PREF(MinEscapeePct)._min)||(tmpi>PREF(MinEscapeePct)._max))
				THROW(Exception,"Minimum done % must be from 1 to 99");
			tmpu = tmpi;
			prefs.set(PREF(MinEscapeePct),tmpu);

			double tmpf=0.0;
			if (!f_live_threshold->read(tmpf))
				THROW(Exception,"Sorry, I don't understand your Live threshold");
			if ((tmpf<PREF(LiveThreshold)._min)||(tmpf>PREF(LiveThreshold)._max))
				THROW(Exception,"Live threshold must be between 0 and 1");
			prefs.set(PREF(LiveThreshold), tmpf);
		}
	};

	class HUDFrame : public Gtk::Frame {
		public:
			Gtk::VScale *vert;
			Gtk::HScale *horiz;
			// XXX colour picker/s

		HUDFrame() : Gtk::Frame("Heads-Up Display") {
			set_border_width(10);
			Gtk::Table* tbl = Gtk::manage(new Gtk::Table(4,3,false));
			Gtk::Label *lbl;

			lbl = Gtk::manage(new Gtk::Label("Vertical position"));
			lbl->set_tooltip_text(PREFDESC(HUDVerticalOffset));
			lbl->set_angle(90);
			tbl->attach(*lbl, 0, 1, 0, 4);

			lbl = Gtk::manage(new Gtk::Label("Horizontal position"));
			lbl->set_tooltip_text(PREFDESC(HUDHorizontalOffset));
			tbl->attach(*lbl, 2, 3, 3, 4);

			vert = Gtk::manage(new Gtk::VScale(0.0, 105.0, 5.0));
			vert->set_digits(0);
			tbl->attach(*vert, 1, 2, 0, 4);

			horiz = Gtk::manage(new Gtk::HScale(0.0, 80.0, 5.0));
			horiz->set_digits(0);
			tbl->attach(*horiz, 2, 3, 2, 3);

			//lbl = Gtk::manage(new Gtk::Label("HUD colour"));
			//tbl->attach(*lbl, 2, 3, 0, 1);

			lbl = Gtk::manage(new Gtk::Label("COLOUR PICKER GOES HERE"));
			tbl->attach(*lbl, 2, 3, 0, 2);
			// XXX.

			add(*tbl);
		}

		void prepare(const Prefs& prefs) {
			horiz->set_value(prefs.get(PREF(HUDHorizontalOffset)));
			vert->set_value(prefs.get(PREF(HUDVerticalOffset)));
			// XXX colours
		}

		void defaults() {
			horiz->set_value(PREF(HUDHorizontalOffset)._default);
			vert->set_value(PREF(HUDVerticalOffset)._default);
			// XXX colours
		}

		void readout(Prefs& prefs) throw(Exception) {
			prefs.set(PREF(HUDHorizontalOffset), horiz->get_value());
			prefs.set(PREF(HUDVerticalOffset), vert->get_value());
			// XXX colours
		}
	};
};

PrefsDialog::PrefsDialog(MainWindow *_mw) : Gtk::Dialog("Preferences", *_mw, true),
	mw(_mw)
{
	add_button("Defaults", RESPONSE_DEFAULTS);
	add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_OK);
	set_default_response(Gtk::ResponseType::RESPONSE_OK);

	Gtk::Box* box = get_vbox();
	threshold = Gtk::manage(new PrefsDialogBits::ThresholdFrame());
	box->pack_start(*threshold);
	hud = Gtk::manage(new PrefsDialogBits::HUDFrame());
	box->pack_start(*hud);
}

int PrefsDialog::run() {
	const Prefs& p = Prefs::getMaster();
	threshold->prepare(p);
	hud->prepare(p);
	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();

		if (result == Gtk::ResponseType::RESPONSE_OK) {
			std::unique_ptr<Prefs> pp = p.getWorkingCopy();
			try {
				threshold->readout(*pp);
				hud->readout(*pp);
			} catch (Exception e) {
				Util::alert(this, e.msg);
				error = true;
				continue;
			}

			if (error) {
				// Any other error cases?
			} else {
				pp->commit();
				// Poke anything that might want to know.
			}
		} else if (result == RESPONSE_DEFAULTS) {
			threshold->defaults();
			hud->defaults();
		}
	} while ((result == RESPONSE_DEFAULTS) ||
			 (error && result == Gtk::ResponseType::RESPONSE_OK));

	return result;
}
