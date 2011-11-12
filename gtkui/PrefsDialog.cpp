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
#include "ColourPanel.h"

#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/enums.h>
#include <gtkmm/scale.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

namespace PrefsDialogBits {

	// Label for our dialog sample text.
	class SampleTextLabel : public ProddableLabel {
		ColourPanel *fgcol, *bgcol;
	public:
		SampleTextLabel() { /* We'll set up text later. */ }

		void set_panels(ColourPanel* fg, ColourPanel* bg) {
			// Urgh, ColourPanel needs to know what to prod, but the prod target also has to get data from either of them. This might be better refactored.
			fgcol=fg;
			bgcol=bg;
		}
		virtual void prod() {
			ostringstream str;
			string FG, BG;
			Gdk::Color fore = fgcol->get_colour(), back = bgcol->get_colour();

			str << hex << setfill('0')
				<< setw(2) << (fore.get_red()>>8)
				<< setw(2) << (fore.get_green()>>8)
				<< setw(2) << (fore.get_blue()>>8);
			FG = str.str();
			// cout << "FG is " << FG << " for " << fore.to_string() << endl; // TEST
			str.seekp(0);

			str << hex << setfill('0')
				<< setw(2) << (back.get_red()>>8)
				<< setw(2) << (back.get_green()>>8)
				<< setw(2) << (back.get_blue()>>8);
			BG = str.str();
			// cout << "BG is " << BG << " for " << back.to_string() << endl; // TEST
			str.seekp(0);

			str << "<span" <<
				" foreground=\"#" << FG << "\"" <<
				" background=\"#" << BG << "\"" <<
				" font_desc=\"" << HUD::font_name << "\""<<
				"> Sample text 01234.567e89</span>";
			// cout << "Markup is: " << str.str() << endl; // TEST
			set_markup(str.str());
		}

	};

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
		ColourPanel *bgcol, *fgcol;
		SampleTextLabel *sample;

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
			vert->set_value_pos(Gtk::PositionType::POS_TOP);
			vert->set_tooltip_text(PREFDESC(HUDVerticalOffset));
			tbl->attach(*vert, 1, 2, 0, 4);

			horiz = Gtk::manage(new Gtk::HScale(0.0, 80.0, 5.0));
			horiz->set_digits(0);
			horiz->set_value_pos(Gtk::PositionType::POS_RIGHT);
			horiz->set_tooltip_text(PREFDESC(HUDHorizontalOffset));
			tbl->attach(*horiz, 2, 3, 2, 3);

			//lbl = Gtk::manage(new Gtk::Label("HUD colour"));
			//tbl->attach(*lbl, 2, 3, 0, 1);

			Gtk::Table* inner = Gtk::manage(new Gtk::Table(2,3,false));
			tbl->attach(*inner, 2, 3, 0, 2);

			sample = Gtk::manage(new SampleTextLabel());
			inner->attach(*sample, 0, 2, 2, 3);

			fgcol = Gtk::manage(new ColourPanel(this, sample));
			fgcol->set_tooltip_text("HUD text colour");
			inner->attach(*fgcol, 0, 1, 0, 1, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL);
			bgcol = Gtk::manage(new ColourPanel(this, sample));
			bgcol->set_tooltip_text("HUD background colour");
			inner->attach(*bgcol, 1, 2, 0, 1, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL);

			sample->set_panels(fgcol, bgcol);

			lbl = Gtk::manage(new Gtk::Label()); // empty, for spacing
			inner->attach(*lbl, 0, 2, 1, 2);

			add(*tbl);
		}

		void prepare(const Prefs& prefs) {
			horiz->set_value(prefs.get(PREF(HUDHorizontalOffset)));
			vert->set_value(prefs.get(PREF(HUDVerticalOffset)));

			Gdk::Color bg,fg;
			if (!bg.set(prefs.get(PREF(HUDBackground))))
				bg.set(PREF(HUDBackground)._default);
			if (!fg.set(prefs.get(PREF(HUDText))))
				fg.set(PREF(HUDText)._default);
			bgcol->set_colour(bg);
			fgcol->set_colour(fg);
			sample->prod();
		}

		void defaults() {
			horiz->set_value(PREF(HUDHorizontalOffset)._default);
			vert->set_value(PREF(HUDVerticalOffset)._default);

			Gdk::Color bg(PREF(HUDBackground)._default);
			Gdk::Color fg(PREF(HUDText)._default);
			bgcol->set_colour(bg);
			bgcol->on_expose_event(0); // Force update
			fgcol->set_colour(fg);
			fgcol->on_expose_event(0); // Force update
			sample->prod();
		}

		void readout(Prefs& prefs) throw(Exception) {
			prefs.set(PREF(HUDHorizontalOffset), horiz->get_value());
			prefs.set(PREF(HUDVerticalOffset), vert->get_value());

			prefs.set(PREF(HUDBackground), bgcol->get_colour().to_string());
			prefs.set(PREF(HUDText), fgcol->get_colour().to_string());
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

	if (result == Gtk::ResponseType::RESPONSE_OK) {
		// We might want to redraw the HUD. Thankfully, this is easy.
		if (mw->hud_active()) {
			mw->toggle_hud();
			mw->toggle_hud();
		}
	}

	return result;
}
