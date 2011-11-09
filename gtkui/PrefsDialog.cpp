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

#include <sstream>

namespace PrefsDialogBits {
	class ThresholdFrame : public Gtk::Frame {
		public:
		// Editable fields:
		Util::HandyEntry<int> *f_init_maxiter, *f_min_done_pct;
		Util::HandyEntry<double> *f_live_threshold;

		ThresholdFrame() : Gtk::Frame("Plot finish threshold tuning") {
			f_init_maxiter = Gtk::manage(new Util::HandyEntry<int>());
			f_min_done_pct = Gtk::manage(new Util::HandyEntry<int>());
			f_live_threshold = Gtk::manage(new Util::HandyEntry<double>());

			set_border_width(10);
			Gtk::Table *tbl = Gtk::manage(new Gtk::Table(3, 2, false));
			Gtk::Label *lbl;

			lbl = Gtk::manage(new Gtk::Label(PREFNAME(InitialMaxIter)));
			lbl->set_tooltip_text(PREFDESC(InitialMaxIter));
			f_init_maxiter->set_tooltip_text(PREFDESC(InitialMaxIter));
			tbl->attach(*lbl, 0, 1, 0, 1);
			tbl->attach(*f_init_maxiter, 1, 2, 0, 1);

			lbl = Gtk::manage(new Gtk::Label("Minimum done %"));
			lbl->set_tooltip_text("Percentage of pixels required to have escaped before a plot is considered finished");
			f_min_done_pct->set_tooltip_text("Percentage of pixels required to have escaped before a plot is considered finished");
			tbl->attach(*lbl, 0, 1, 1, 2);
			tbl->attach(*f_min_done_pct, 1, 2, 1, 2);

			lbl = Gtk::manage(new Gtk::Label("Live threshold"));
			lbl->set_tooltip_text("The smallest number of pixels which may be escaping in order for a plot to be considered finished");
			f_live_threshold->set_tooltip_text("The smallest number of pixels which may be escaping in order for a plot to be considered finished");
			tbl->attach(*lbl, 0, 1, 2, 3);
			tbl->attach(*f_live_threshold, 1, 2, 2, 3);

			add(*tbl);
		}

		void prepare(Prefs& prefs) {
			f_init_maxiter->update(prefs.get(PREF(InitialMaxIter)));
			f_min_done_pct->update(prefs.min_escapee_pct());
			f_live_threshold->update(prefs.plot_live_threshold_fract(),4);
		}

		void readout(Prefs& prefs) throw(Exception) {
			unsigned tmpu;
			int tmpi=0;

			if (!f_init_maxiter->read(tmpi))
				throw Exception("Sorry, I don't understand your initial maxiter");
			if ((tmpi < PREF(InitialMaxIter)._min) || (tmpi > PREF(InitialMaxIter)._max))
				throw Exception("Initial maxiter must be at least 2");
			tmpu = tmpi;
			prefs.set(PREF(InitialMaxIter), tmpu);

			if (!f_min_done_pct->read(tmpi))
				throw Exception("Sorry, I don't understand your Minimum done %");
			if ((tmpi<1)||(tmpi>99))
				throw Exception("Minimum done % must be from 1 to 99");
			tmpu = tmpi;
			prefs.min_escapee_pct(tmpu);

			double tmpf;
			if (!f_live_threshold->read(tmpf))
				throw Exception("Sorry, I don't understand your Live threshold");
			if ((tmpf<0.0)||(tmpf>1.0))
				throw Exception("Live threshold must be between 0 and 1");
			prefs.plot_live_threshold_fract(tmpf);
		}
	};
};

PrefsDialog::PrefsDialog(MainWindow *_mw) : Gtk::Dialog("Preferences", _mw, true),
	mw(_mw)
{
	add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_ACCEPT);

	Gtk::Box* box = get_vbox();
	threshold = Gtk::manage(new PrefsDialogBits::ThresholdFrame());
	box->pack_start(*threshold);
}

int PrefsDialog::run() {
	Prefs& p = Prefs::getDefaultInstance();
	threshold->prepare(p);
	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();

		if (result == GTK_RESPONSE_ACCEPT) {
			try {
				threshold->readout(p);
			} catch (Exception e) {
				Util::alert(mw, e.msg);
				error = true;
				continue;
			}

			if (error) {
				// Any other error cases?
			} else {
				p.commit();
				// Poke anything that might want to know.
			}
		}
	} while (error && result == GTK_RESPONSE_ACCEPT);

	return result;
}
