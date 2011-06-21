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

#ifndef PARAMS_DIALOG_H
#define PARAMS_DIALOG_H

#include "MainWindow.h"
#include "Plot2.h"
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

class ParamsDialog : public Gtk::Dialog {
	protected:
		MainWindow* mw;
		// Dialog fields:
		Gtk::Entry *f_c_re, *f_c_im, *f_size_re;

	public:
		ParamsDialog(MainWindow *_mw);
		int run(); // Returns the Gtk::ResponseType::RESPONSE_* code selected. If ACCEPT, the MainWindow params have been directly updated.
};

#endif

