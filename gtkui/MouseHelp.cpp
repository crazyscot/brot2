/*
    MouseHelp: brot2 mouse controls help window
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

#include "MouseHelp.h"
#include "MainWindow.h"
#include <gdk/gdkkeysyms.h>

MouseHelpWindow::MouseHelpWindow(MainWindow& parent, Prefs& prefs) : myparent(parent), myprefs(prefs) {
	set_title("Controls");
	// ... XXX put controls here XXX
}
// XXX "prod" or "update" when prefs changed.

bool MouseHelpWindow::close() {
	hide();
	myparent.optionsMenu()->set_mousehelp(false);
	return true;
}

bool MouseHelpWindow::on_delete_event(GdkEventAny *evt) {
	(void)evt;
	return close();
}

bool MouseHelpWindow::on_key_release_event(GdkEventKey *evt) {
	switch(evt->keyval) {
		case GDK_Escape:
			return close();
	}
	return false;
}
