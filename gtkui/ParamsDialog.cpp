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
	add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_ACCEPT);

	Gtk::Box* box = get_vbox();
	Gtk::Table *tbl = Gtk::manage(new Gtk::Table(3,2));

	f_c_re = Gtk::manage(new Gtk::Entry());
	f_c_im = Gtk::manage(new Gtk::Entry());
	f_size_re = Gtk::manage(new Gtk::Entry());

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
}

static void update_entry_float(Gtk::Entry& entry, const Fractal::Value val)
{
	std::ostringstream tmp;
	tmp.precision(MAXIMAL_DECIMAL_PRECISION);
	tmp << val;
	entry.set_text(tmp.str().c_str());
}

static bool read_entry_float(const Gtk::Entry& entry, Fractal::Value& val_out)
{
	Glib::ustring raw = entry.get_text();
	std::istringstream tmp(raw, std::istringstream::in);
	tmp.precision(MAXIMAL_DECIMAL_PRECISION);
	Fractal::Value rv=0;

	tmp >> rv;
	if (tmp.fail())
		return false;

	val_out = rv;
	return true;
}

int ParamsDialog::run() {
	const Fractal::Point& ctr = mw->get_centre();
	update_entry_float(*f_c_re, real(ctr));
	update_entry_float(*f_c_im, imag(ctr));
	update_entry_float(*f_size_re, real(mw->get_size()));
	show_all();

	bool error;
	gint result;
	do {
		error = false;
		result = Gtk::Dialog::run();
		Fractal::Point new_ctr, new_size;

		if (result == GTK_RESPONSE_ACCEPT) {
			Fractal::Value res=0;
			if (read_entry_float(*f_c_re, res))
				new_ctr.real(res);
			else error = true;
			if (read_entry_float(*f_c_im, res))
				new_ctr.imag(res);
			else error = true;
			if (read_entry_float(*f_size_re, res))
				new_size.real(res);
			else error = true;
			// imaginary axis length is implicit.

			if (error)
				Util::alert(mw, "Sorry, I could not parse that; care to try again?");
			else
				mw->update_params(new_ctr, new_size);
		}
	} while (error && result == GTK_RESPONSE_ACCEPT);

	return result;
}
