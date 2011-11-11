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

#include "ParamsDialog.h"
#include "MainWindow.h"
#include "Fractal.h"
#include "misc.h"
#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>

#include <sstream>

// TODO Make this non-modal ???

ParamsDialog::ParamsDialog(MainWindow *_mw) : Gtk::Dialog("Parameters", _mw, true),
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
	f_size_re = Gtk::manage(new Util::HandyEntry<Fractal::Value>());
	f_size_re->set_activates_default(true);

	Gtk::Label* label;

	label = Gtk::manage(new Gtk::Label("Centre Real (x) "));
	label->set_alignment(1, 0.5);
	tbl->attach(*label, 0, 1, 0, 1);
	tbl->attach(*f_c_re, 1, 2, 0, 1);

	label = Gtk::manage(new Gtk::Label("Centre Imaginary (y) "));
	label->set_alignment(1, 0.5);
	tbl->attach(*label, 0, 1, 1, 2);
	tbl->attach(*f_c_im, 1, 2, 1, 2);

	label = Gtk::manage(new Gtk::Label("Real (x) axis length "));
	label->set_alignment(1, 0.5);
	tbl->attach(*label, 0, 1, 2, 3);
	tbl->attach(*f_size_re, 1, 2, 2, 3);
	// Don't bother with imaginary axis length, it's implicit from the aspect ratio.

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
	f_size_re->update(real(mw->get_size()), AXIS_LENGTH_PRECISION);
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
			if (f_size_re->read(res)) {
				new_size.real(res);
			} else {
				if (!error) // don't flood too many messages
					Util::alert(this, "Sorry, I could not parse that axis length.");
				error=true;
			}
			// imaginary axis length is implicit.

			if (!error)
				mw->update_params(new_ctr, new_size);
		}
	} while (error && result == Gtk::ResponseType::RESPONSE_OK);

	return result;
}
