/*
    MainWindow: GTK+ (gtkmm) main window for brot2
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

#include "MainWindow.h"
#include "Menus.h"
#include "Canvas.h"
#include "misc.h"
#include "config.h"

#include <stdlib.h>
#include <gtkmm/main.h>
#include <gdk/gdkkeysyms.h>
#include <pangomm.h>

MainWindow::MainWindow() : Gtk::Window() {
	set_title(PACKAGE_NAME); // Renderer will update this
	vbox = Gtk::manage(new Gtk::VBox()); // XXX false,1 ?
	vbox->set_border_width(1);
	add(*vbox);

	menubar = Gtk::manage(new menus::Menus());
	vbox->pack_start(*menubar, false, false, 0);

	canvas = Gtk::manage(new Canvas());
	vbox->pack_start(*canvas, true, true, 0);

	progbar = Gtk::manage(new Gtk::ProgressBar());
	progbar->set_text("Initialising");
	progbar->set_ellipsize(Pango::ELLIPSIZE_END);
	progbar->set_pulse_step(0.1);
	vbox->pack_end(*progbar, false, false, 0);
}

MainWindow::~MainWindow() {
	// vbox is managed so auto-deletes
}

void MainWindow::do_zoom(enum Zoom z) {
	std::cerr << "DO_ZOOM WRITEME" << std::endl;
	(void)z;
	// XXX WRITEME
}

bool MainWindow::on_key_release_event(GdkEventKey *event) {
	switch(event->keyval) {
	case GDK_KP_Add:
		do_zoom(ZOOM_IN);
		return true;
	case GDK_KP_Subtract:
		do_zoom(ZOOM_OUT);
		return true;
	}
	return false;
}

bool MainWindow::on_delete_event(GdkEventAny * UNUSED(e)) {
	Gtk::Main::instance()->quit(); // NORETURN
	return true;
}
