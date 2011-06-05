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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <iostream>
#include <gtkmm/window.h>
#include <gtkmm/menubar.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/progressbar.h>

class MainWindow : public Gtk::Window {
	Gtk::VBox *vbox; // Main layout widget
	Gtk::MenuBar *menubar;
	Gtk::DrawingArea *canvas;
	Gtk::ProgressBar *progbar;

	// XXX Gtk::UIManager *uimgr;

public:
	enum Zoom {
		REDRAW_ONLY,
		ZOOM_IN,
		ZOOM_OUT,
	};

	MainWindow();
	virtual ~MainWindow();
    virtual bool on_key_release_event(GdkEventKey *);
    virtual bool on_delete_event(GdkEventAny *);

    void do_zoom(enum Zoom z);
};

#endif /* MAINWINDOW_H_ */
