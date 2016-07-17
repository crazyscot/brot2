/*
    MovieWindow: brot2 movie making control
    Copyright (C) 2016 Ross Younger

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
#include "MovieWindow.h"
#include "MainWindow.h"
#include "Fractal.h"
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

MovieWindow::MovieWindow(MainWindow& _mw, std::shared_ptr<const Prefs> prefs) : mw(_mw), _prefs(prefs)
{
	set_title("Movie");

	// TODO: Controls etc go here.

	hide();
}

MovieWindow::~MovieWindow() {
}

void MovieWindow::reset() {
	// TODO: Clears down everything we have stored.
}

bool MovieWindow::close() {
	hide();
	reset();
	return true;
}

bool MovieWindow::on_delete_event(GdkEventAny *evt) {
	(void)evt;
	return close();
}
