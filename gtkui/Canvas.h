/*
    Canvas: GTK+ (gtkmm) drawing area for brot2
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


#ifndef CANVAS_H_
#define CANVAS_H_

class MainWindow;

#include <gtkmm/drawingarea.h>
#include <cairomm/cairomm.h>

class Canvas: public Gtk::DrawingArea {
	MainWindow *main; // Our parent
	Cairo::RefPtr<Cairo::Surface> surface; // Staging area - we render plots here, then to the window when exposed
public:
	Canvas(MainWindow *parent);
	virtual ~Canvas();

    virtual bool on_button_press_event(GdkEventButton * evt);
    virtual bool on_button_release_event(GdkEventButton * evt);
    virtual bool on_motion_notify_event(GdkEventMotion * evt);
    virtual bool on_expose_event(GdkEventExpose * evt);
    virtual bool on_configure_event(GdkEventConfigure * evt);
};

#endif /* CANVAS_H_ */
