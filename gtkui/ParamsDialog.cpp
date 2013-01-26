/*
    ParamsDialog: brot2 fractal parameters configuration dialog
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
#include "ParamsDialog.h"
#include "MainWindow.h"
#include "Fractal.h"
#include "misc.h"
#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/frame.h>

#include <sstream>

// TODO Make this non-modal ???

class ZoomControl : public Gtk::Frame {
		MainWindow *_mw;
		Gtk::RadioButtonGroup grp;
		Gtk::Table *tbl;
		Util::HandyEntry<Fractal::Value> *field;

		Fractal::Value shadow; // Need to compute real axis length from this.

		enum mode_t {
			RE_AX, // real axis len
			IM_AX, // imag
			RE_PIX, // real pixel size
			IM_PIX, // imag
			ZOOM // zoom factor
		} mode;

		void set(Fractal::Value v, mode_t m) {
			shadow = v;
			mode = m;
			field->update(v);
		}

		bool get_raw (Fractal::Value& res) const {
			return field->read(res);
		}

	public:
		// sets, as real axis length.
		void set(Fractal::Value v) {
			set(v, RE_AX);
			// todo some day? allow something other than RE_AX inbound.
		}

		// reads out, converted to real axis length.
		bool get (Fractal::Value& res) const {
			Fractal::Value t = 0;
			if (!get_raw(t))
				return false;

			switch(mode) {
				case RE_AX:  res = t; break;
				case IM_AX:  res = im_to_re(t); break;
				case RE_PIX: res = pix_to_ax(t); break;
				case IM_PIX: res = pix_to_ax(im_to_re(t)); break;
				case ZOOM:   res = zoom_to_ax(t); break;
			}
			return true;
		}

		ZoomControl(MainWindow *mw) : Gtk::Frame("Zoom control"), _mw(mw) {
			mode = RE_AX;
			set_border_width(10);
			tbl = Gtk::manage(new Gtk::Table(2,4));
			Gtk::RadioButton *b;
#define RADIO(LABEL,X1,Y1,MODE) do {							\
	b = Gtk::manage(new Gtk::RadioButton(grp, LABEL));			\
	tbl->attach(*b, X1, X1+1, Y1, Y1+1);						\
	b->signal_toggled().connect(sigc::bind<Gtk::ToggleButton*, mode_t>(	\
				sigc::mem_fun(*this, &ZoomControl::do_toggle), 	\
				b, MODE));										\
} while(0)

			RADIO("Real (x) axis length", 0, 0, RE_AX);
			RADIO("Imag (y) axis length", 1, 0, IM_AX);
			RADIO("Real (x) pixel size", 0, 1, RE_PIX);
			RADIO("Imag (y) pixel size", 1, 1, IM_PIX);
			RADIO("Zoom factor", 0, 2, ZOOM);

			field = Gtk::manage(new Util::HandyEntry<Fractal::Value>());
			field->set_activates_default(true);
			tbl->attach(*field, 0, 2, 3, 4);

			add(*tbl);
		}

	protected:

		inline Fractal::Value re_to_im(Fractal::Value in) const {
			return in * _mw->get_rheight() / _mw->get_rwidth();
		}
		inline Fractal::Value im_to_re(Fractal::Value in) const {
			return in * _mw->get_rwidth() / _mw->get_rheight();
		}
		inline Fractal::Value pix_to_ax(Fractal::Value in) const {
			return in * _mw->get_rwidth();
		}
		inline Fractal::Value ax_to_pix(Fractal::Value in) const {
			return in / _mw->get_rwidth();
		}
		inline Fractal::Value zoom_to_ax(Fractal::Value in) const {
			// FIXME Law of Demeter violation
			return _mw->get_plot().zoom_to_axis(in);
		}
		inline Fractal::Value ax_to_zoom(Fractal::Value in) const {
			// FIXME Law of Demeter violation
			return _mw->get_plot().axis_to_zoom(in);
		}

		void do_toggle(Gtk::ToggleButton*b, mode_t newmode) {
			if (!b->get_active()) return;
			Fractal::Value t = 0;
			if (!get_raw(t)) {
				mode = newmode;
				return; // cannot parse, don't attempt to scale
			}

			switch(mode) {
				case RE_AX:
					switch(newmode) {
						case RE_AX: break;
						case IM_AX:
							shadow = re_to_im(t); break;
						case RE_PIX:
							shadow = ax_to_pix(t); break;
						case IM_PIX:
							shadow = re_to_im(ax_to_pix(t)); break;
						case ZOOM:
							shadow = ax_to_zoom(t); break;
					}
					break;

				case IM_AX:
					switch(newmode) {
						case RE_AX:
							shadow = im_to_re(t); break;
						case IM_AX:
							break;
						case RE_PIX:
							shadow = im_to_re(ax_to_pix(t)); break;
						case IM_PIX:
							shadow = ax_to_pix(t); break;
						case ZOOM:
							shadow = ax_to_zoom(im_to_re(t)); break;
					}
					break;

				case RE_PIX:
					switch(newmode) {
						case RE_AX:
							shadow = pix_to_ax(t); break;
						case IM_AX:
							shadow = re_to_im(pix_to_ax(t)); break;
						case RE_PIX:
							break;
						case IM_PIX:
							shadow = re_to_im(t); break;
						case ZOOM:
							shadow = ax_to_zoom(pix_to_ax(t)); break;
					}
					break;

				case IM_PIX:
					switch(newmode) {
						case RE_AX:
							shadow = im_to_re(pix_to_ax(t)); break;
						case IM_AX:
							shadow = pix_to_ax(t); break;
						case RE_PIX:
							shadow = im_to_re(t); break;
						case IM_PIX:
							break;
						case ZOOM:
							shadow = ax_to_zoom(pix_to_ax(im_to_re(t))); break;
					}
					break;

				case ZOOM:
					switch(newmode) {
						case RE_AX:
							shadow = zoom_to_ax(t); break;
						case IM_AX:
							shadow = re_to_im(zoom_to_ax(t)); break;
						case RE_PIX:
							shadow = ax_to_pix(zoom_to_ax(t)); break;
						case IM_PIX:
							shadow = re_to_im(ax_to_pix(zoom_to_ax(t))); break;
						case ZOOM:
							break;
					}
					break;
			}
			set(shadow);
			mode = newmode;
		}
};

ParamsDialog::ParamsDialog(MainWindow *_mw) : Gtk::Dialog("Parameters", *_mw, true),
	mw(_mw)
{
	add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_OK);

	Gtk::Box* box = get_vbox();
	Gtk::Table *tbl = Gtk::manage(new Gtk::Table(3,2));

	f_c_re = Gtk::manage(new Util::HandyEntry<Fractal::Value>());
	f_c_re->set_activates_default(true);
	f_c_im = Gtk::manage(new Util::HandyEntry<Fractal::Value>());
	f_c_im->set_activates_default(true);

	zc = Gtk::manage(new ZoomControl(mw));

	Gtk::Label* label;

	label = Gtk::manage(new Gtk::Label("Centre Real (x) "));
	label->set_alignment(1, 0.5);
	tbl->attach(*label, 0, 1, 0, 1);
	tbl->attach(*f_c_re, 1, 2, 0, 1);

	label = Gtk::manage(new Gtk::Label("Centre Imaginary (y) "));
	label->set_alignment(1, 0.5);
	tbl->attach(*label, 0, 1, 1, 2);
	tbl->attach(*f_c_im, 1, 2, 1, 2);

	tbl->attach(*zc, 0, 2, 2, 3);

	box->pack_start(*tbl);
	set_default_response(Gtk::ResponseType::RESPONSE_OK);
}

int ParamsDialog::run() {
	const Fractal::Point& ctr = mw->get_centre();
	/* LP#783087:
	 * Compute the size of a pixel in fractal units, then work out the
	 * decimal precision required to express that, plus 1 for a safety
	 * margin. */
	const Fractal::Value xpixsize = real(mw->get_size()) / mw->get_rwidth();
	const Fractal::Value ypixsize = imag(mw->get_size()) / mw->get_rheight();
	const int clampx = 1+ceill(0-log10(xpixsize)),
			  clampy = 1+ceill(0-log10(ypixsize));

	f_c_re->update(real(ctr), clampx);
	f_c_im->update(imag(ctr), clampy);
	zc->set(real(mw->get_size())); // Real axis length is the default option.
	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();
		Fractal::Point new_ctr, new_size;

		if (result == Gtk::ResponseType::RESPONSE_OK) {
			Fractal::Value res=0;
			if (f_c_re->read(res)) {
				new_ctr.real(res);
			} else {
				Util::alert(this, "Sorry, I could not parse that real centre.");
				error=true;
			}
			if (f_c_im->read(res)) {
				new_ctr.imag(res);
			} else {
				if (!error) // don't flood too many messages
					Util::alert(this, "Sorry, I could not parse that imaginary centre.");
				error=true;
			}
			if (zc->get(res)) {
				new_size.real(res);
			} else {
				if (!error) // don't flood too many messages
					Util::alert(this, "Sorry, I could not parse that zoom factor.");
				error=true;
			}

			if (!error)
				mw->update_params(new_ctr, new_size);
		}
	} while (error && result == Gtk::ResponseType::RESPONSE_OK);

	return result;
}
