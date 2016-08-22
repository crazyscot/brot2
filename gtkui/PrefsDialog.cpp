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

#include "png.h" // must be first, see launchpad 218409
#include "PrefsDialog.h"
#include "MainWindow.h"
#include "gtkutil.h"
#include "Prefs.h"
#include "Exception.h"
#include "ColourPanel.h"
#include "BaseHUD.h"

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
using namespace BrotPrefs;

namespace PrefsDialogBits {

	// Label for our dialog sample text.
	class SampleTextLabel : public ProddableLabel {
		ColourPanel *fgcol, *bgcol;
		Gtk::HScale *fontsz;
	public:
		SampleTextLabel() : fgcol(0), bgcol(0), fontsz(0) { /* We'll set up text later. */ }

		void set_panels(ColourPanel* fg, ColourPanel* bg) {
			// Urgh, ColourPanel needs to know what to prod, but the prod target also has to get data from either of them. This might be better refactored.
			fgcol=fg;
			bgcol=bg;
		}
		void set_fontsize(Gtk::HScale* hs) {
			fontsz = hs;
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

			// But alas, neither pango nor gtk (currently) allow us to have
			// a text string with alpha colouring.
			// TODO: Another day, refactor this to use a DrawingArea and
			// invoke cairo directly to achieve an alphaful preview.
			// TODO: Show outline text, if that is selected.
			str << "<span" <<
				" foreground=\"#" << FG << "\"" <<
				" background=\"#" << BG << "\"" <<
				" font_family=\"" << BaseHUD::font_name << "\"" <<
				" size=\"" << fontsz->get_value()*PANGO_SCALE << "\"" <<
				" weight=\"bold\"" <<
				"> Sample 0123 </span>";
			// cout << "Markup is: " << str.str() << endl; // TEST
			set_markup(str.str());
		}

	};

	/* An HScale which "constrains" another.
	 * In this case, other must always be at least 1 larger than this one. */
	class ConstrainingHScale : public Gtk::HScale {
		Gtk::HScale *other;
		public:
			ConstrainingHScale(Gtk::HScale *p) : other(p) { }
			ConstrainingHScale(Gtk::HScale *p,
					double min, double max, double step) :
				Gtk::HScale(min,max,step), other(p) { }

			virtual void on_value_changed() {
				other->get_adjustment()->set_lower(get_value()+1);
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

		void readout(Prefs& prefs) throw(PrefsException) {
			unsigned tmpu;
			int tmpi=0;

			if (!f_init_maxiter->read(tmpi))
				THROW(PrefsException,"Sorry, I don't understand your initial maxiter");
			if ((tmpi < PREF(InitialMaxIter)._min) || (tmpi > PREF(InitialMaxIter)._max))
				THROW(PrefsException,"Initial maxiter must be at least 2");
			tmpu = tmpi;
			prefs.set(PREF(InitialMaxIter), tmpu);

			if (!f_min_done_pct->read(tmpi))
				THROW(PrefsException,"Sorry, I don't understand your Minimum done %");
			if ((tmpi<PREF(MinEscapeePct)._min)||(tmpi>PREF(MinEscapeePct)._max))
				THROW(PrefsException,"Minimum done % must be from 1 to 99");
			tmpu = tmpi;
			prefs.set(PREF(MinEscapeePct),tmpu);

			double tmpf=0.0;
			if (!f_live_threshold->read(tmpf))
				THROW(PrefsException,"Sorry, I don't understand your Live threshold");
			if ((tmpf<PREF(LiveThreshold)._min)||(tmpf>PREF(LiveThreshold)._max))
				THROW(PrefsException,"Live threshold must be between 0 and 1");
			prefs.set(PREF(LiveThreshold), tmpf);
		}
	};

	class MiscFrame : public Gtk::Frame {
		public:
		// Editable fields:
		Util::HandyEntry<int> *f_max_threads;
		Util::HandyEntry<int> *f_tile_size;

		MiscFrame() : Gtk::Frame("Miscellaneous") {
			f_max_threads = Gtk::manage(new Util::HandyEntry<int>());
			f_max_threads->set_activates_default(true);
            f_tile_size = Gtk::manage(new Util::HandyEntry<int>());
			f_tile_size->set_activates_default(true);

			set_border_width(10);
			Gtk::Table *tbl = Gtk::manage(new Gtk::Table(2/*r*/, 2/*c*/, false));
			Gtk::Label *lbl;

			lbl = Gtk::manage(new Gtk::Label(PREFNAME(MaxPlotThreads)));
			lbl->set_tooltip_text(PREFDESC(MaxPlotThreads));
			f_max_threads->set_tooltip_text(PREFDESC(MaxPlotThreads));
			tbl->attach(*lbl, 0, 1, 0, 1);
			tbl->attach(*f_max_threads, 1, 2, 0, 1);

			lbl = Gtk::manage(new Gtk::Label(PREFNAME(TileSize)));
			lbl->set_tooltip_text(PREFDESC(TileSize));
			f_tile_size->set_tooltip_text(PREFDESC(TileSize));
			tbl->attach(*lbl, 0, 1, 1, 2);
			tbl->attach(*f_tile_size, 1, 2, 1, 2);

			add(*tbl);
		}

		void prepare(const Prefs& prefs) {
			f_max_threads->update(prefs.get(PREF(MaxPlotThreads)));
            f_tile_size->update(prefs.get(PREF(TileSize)));
		}

		void defaults() {
			f_max_threads->update(PREF(MaxPlotThreads)._default);
			f_tile_size->update(PREF(TileSize)._default);
		}

		void readout(Prefs& prefs) throw(PrefsException) {
			unsigned tmpu;
			int tmpi=0;

			if (!f_max_threads->read(tmpi))
				THROW(PrefsException,"Sorry, I don't understand your max CPU threads");
			if ((tmpi < PREF(MaxPlotThreads)._min) || (tmpi > PREF(MaxPlotThreads)._max))
				THROW(PrefsException,"Max CPU threads must be at least 0");
			tmpu = tmpi;
			prefs.set(PREF(MaxPlotThreads), tmpu);

			if (!f_tile_size->read(tmpi))
				THROW(PrefsException,"Sorry, I don't understand your render tile size");
			if ((tmpi < PREF(TileSize)._min) || (tmpi > PREF(TileSize)._max))
				THROW(PrefsException,"Tile size must be at least 10");
			tmpu = tmpi;
			prefs.set(PREF(TileSize), tmpu);
		}
	};

	class HUDFrame : public Gtk::Frame {
	public:
		Gtk::VScale *vert;
		Gtk::HScale *horiz, *rightmarg, *fontsize;
		Gtk::HScale *nalpha; // transparency 0.0-0.5, so alpha is 1.0 - nalpha.
		Gtk::Adjustment *hadjust, *radjust;
		Gtk::CheckButton *outline, *show_zoom;
		ColourPanel *bgcol, *fgcol;
		SampleTextLabel *sample;

		HUDFrame() : Gtk::Frame("Heads-Up Display"), hadjust(0), radjust(0) {
			set_border_width(10);
			Gtk::Table* tbl = Gtk::manage(new Gtk::Table(7,2,false));
			Gtk::Label *lbl;

			{
				// Vertical position
				Gtk::Table* inner = Gtk::manage(new Gtk::Table(1,2,false));
				tbl->attach(*inner, 0, 1, 0, 5);

				lbl = Gtk::manage(new Gtk::Label("Vertical position (%)"));
				lbl->set_tooltip_text(PREFDESC(HUDVerticalOffset));
				lbl->set_angle(90);
				inner->attach(*lbl, 0, 1, 0, 3);

				vert = Gtk::manage(new Gtk::VScale(0.0, 105.0, 5.0));
				vert->set_digits(0);
				vert->set_value_pos(Gtk::PositionType::POS_LEFT);
				vert->set_tooltip_text(PREFDESC(HUDVerticalOffset));
				inner->attach(*vert, 1, 2, 0, 3);
			}
			{
				// Right margin
				Gtk::Table* inner = Gtk::manage(new Gtk::Table(2,1,false));
				tbl->attach(*inner, 0, 2, 6, 7);

				lbl = Gtk::manage(new Gtk::Label("Right margin (%)"));
				lbl->set_tooltip_text(PREFDESC(HUDRightMargin));
				inner->attach(*lbl, 0, 1, 1, 2);
				rightmarg = Gtk::manage(new Gtk::HScale(1.0, 105.0, 5.0));
				rightmarg->set_digits(0);
				rightmarg->set_value_pos(Gtk::PositionType::POS_TOP);
				rightmarg->set_tooltip_text(PREFDESC(HUDRightMargin));
				inner->attach(*rightmarg, 0, 1, 0, 1);
			}
			{
				// Horizontal position
				// Note ordering - we need rightmarg to be set up by now
				Gtk::Table* inner = Gtk::manage(new Gtk::Table(2,1,false));
				tbl->attach(*inner, 0, 2, 5, 6);

				lbl = Gtk::manage(new Gtk::Label("Horizontal position (%)"));
				lbl->set_tooltip_text(PREFDESC(HUDHorizontalOffset));
				inner->attach(*lbl, 0, 1, 1, 2);
				horiz = Gtk::manage(new ConstrainingHScale(rightmarg, 0.0, 104.0, 5.0));
				horiz->set_digits(0);
				horiz->set_value_pos(Gtk::PositionType::POS_TOP);
				horiz->set_tooltip_text(PREFDESC(HUDHorizontalOffset));
				inner->attach(*horiz, 0, 1, 0, 1);
			}

			{ // Transparency widget
				Gtk::Table* inner = Gtk::manage(new Gtk::Table(2,1,false));
				tbl->attach(*inner, 1, 2, 1, 2);

				lbl = Gtk::manage(new Gtk::Label("Transparency"));
				lbl->set_tooltip_text(PREFDESC(HUDTransparency));
				inner->attach(*lbl, 0, 1, 0, 1);

				nalpha = Gtk::manage(new Gtk::HScale(0.0, 0.6, 0.10));
				nalpha->set_digits(1);
				nalpha->set_value_pos(Gtk::PositionType::POS_RIGHT);
				nalpha->set_tooltip_text(PREFDESC(HUDTransparency));
				inner->attach(*nalpha, 0, 1, 1, 2);
			}

			{ // Colour pickers and sample text
				Gtk::Table* inner = Gtk::manage(new Gtk::Table(2,3,false));
				tbl->attach(*inner, 1, 2, 0, 1);

				sample = Gtk::manage(new SampleTextLabel());
				inner->attach(*sample, 0, 2, 1, 2);

				fgcol = Gtk::manage(new ColourPanel(this, sample));
				fgcol->set_tooltip_text("HUD text colour");
				inner->attach(*fgcol, 0, 1, 0, 1, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL);
				bgcol = Gtk::manage(new ColourPanel(this, sample));
				bgcol->set_tooltip_text("HUD background colour");
				inner->attach(*bgcol, 1, 2, 0, 1, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL);

				sample->set_panels(fgcol, bgcol);

				lbl = Gtk::manage(new Gtk::Label()); // empty, for spacing
				inner->attach(*lbl, 0, 2, 2, 3);
			}

			{ // Font size widget
				Gtk::Table* inner = Gtk::manage(new Gtk::Table(2,1,false));
				tbl->attach(*inner, 1, 2, 2, 3);

				lbl = Gtk::manage(new Gtk::Label("Font size"));
				inner->attach(*lbl, 0, 1, 0, 1);
				fontsize = Gtk::manage(new Gtk::HScale(6.0, 26.0, 2.0));
				fontsize->set_digits(0);
				fontsize->set_value_pos(Gtk::PositionType::POS_RIGHT);
				fontsize->set_tooltip_text(PREFDESC(HUDFontSize));
				inner->attach(*fontsize, 0, 1, 1, 2);

				fontsize->signal_value_changed().connect(sigc::mem_fun(sample, &SampleTextLabel::prod));
				sample->set_fontsize(fontsize);
			}

			outline = Gtk::manage(new Gtk::CheckButton("Outline text"));
			tbl->attach(*outline, 1, 2, 3, 4);

			show_zoom = Gtk::manage(new Gtk::CheckButton("Show \"Zoom:\""));
			tbl->attach(*show_zoom, 1, 2, 4, 5);

			add(*tbl);
		}

		void prepare(const Prefs& prefs) {
			horiz->set_value(prefs.get(PREF(HUDHorizontalOffset)));
			vert->set_value(prefs.get(PREF(HUDVerticalOffset)));
			nalpha->set_value(prefs.get(PREF(HUDTransparency)));
			rightmarg->set_value(prefs.get(PREF(HUDRightMargin)));
			fontsize->set_value(prefs.get(PREF(HUDFontSize)));
			outline->set_active(prefs.get(PREF(HUDOutlineText)));
			show_zoom->set_active(prefs.get(PREF(HUDShowZoom)));

			Gdk::Color bg,fg;
			if (!bg.set(prefs.get(PREF(HUDBackgroundColour))))
				bg.set(PREF(HUDBackgroundColour)._default);
			if (!fg.set(prefs.get(PREF(HUDTextColour))))
				fg.set(PREF(HUDTextColour)._default);
			bgcol->set_colour(bg);
			fgcol->set_colour(fg);
			sample->prod();
		}

		void defaults() {
			horiz->set_value(PREF(HUDHorizontalOffset)._default);
			vert->set_value(PREF(HUDVerticalOffset)._default);
			nalpha->set_value(PREF(HUDTransparency)._default);
			rightmarg->set_value(PREF(HUDRightMargin)._default);
			fontsize->set_value(PREF(HUDFontSize)._default);
			outline->set_active(PREF(HUDOutlineText)._default);
			show_zoom->set_active(PREF(HUDShowZoom)._default);

			Gdk::Color bg(PREF(HUDBackgroundColour)._default);
			Gdk::Color fg(PREF(HUDTextColour)._default);
			bgcol->set_colour(bg);
			bgcol->on_expose_event(0); // Force update
			fgcol->set_colour(fg);
			fgcol->on_expose_event(0); // Force update
			sample->prod();
		}

		void readout(Prefs& prefs) throw(PrefsException) {
			prefs.set(PREF(HUDHorizontalOffset), horiz->get_value());
			prefs.set(PREF(HUDVerticalOffset), vert->get_value());
			prefs.set(PREF(HUDTransparency), nalpha->get_value());
			prefs.set(PREF(HUDRightMargin), rightmarg->get_value());
			prefs.set(PREF(HUDFontSize), fontsize->get_value());
			prefs.set(PREF(HUDOutlineText), outline->get_active());
			prefs.set(PREF(HUDShowZoom), show_zoom->get_active());

			prefs.set(PREF(HUDBackgroundColour), bgcol->get_colour().to_string());
			prefs.set(PREF(HUDTextColour), fgcol->get_colour().to_string());
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
	miscbits = Gtk::manage(new PrefsDialogBits::MiscFrame());
	box->pack_start(*miscbits);
}

int PrefsDialog::run() {
	std::shared_ptr<const Prefs> p = Prefs::getMaster();
	threshold->prepare(*p);
	hud->prepare(*p);
	miscbits->prepare(*p);
	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();

		if (result == Gtk::ResponseType::RESPONSE_OK) {
			std::shared_ptr<Prefs> pp = p->getWorkingCopy();
			try {
				threshold->readout(*pp);
				hud->readout(*pp);
				miscbits->readout(*pp);
			} catch (PrefsException& e) {
				Util::alert(this, e.msg);
				error = true;
				continue;
			}

			if (error) {
				// Any other error cases?
			} else {
				// Do we need to change the size of the main worker ThreadPool?
				if (BrotPrefs::threadpool_size(pp) != BrotPrefs::threadpool_size(p) )
					mw->resize_threadpool(BrotPrefs::threadpool_size(pp));

				pp->commit();
				// Poke anything that might want to know.
			}
		} else if (result == RESPONSE_DEFAULTS) {
			threshold->defaults();
			hud->defaults();
			miscbits->defaults();
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
