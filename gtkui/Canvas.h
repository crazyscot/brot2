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
#include "Fractal.h"

#include "misc.h"
BROT2_GTKMM_BEFORE
#include <gtkmm/drawingarea.h>
BROT2_GTKMM_AFTER
#include <cairomm/cairomm.h>

class Canvas : public Gtk::DrawingArea {
	friend class MainWindow;
	MainWindow *main; // Our parent
	Cairo::RefPtr<Cairo::ImageSurface> surface; // Staging area - we render plots here, then to the window when exposed

	bool end_dragrect(gdouble x, gdouble y);

public:
	Canvas(MainWindow *parent);
	virtual ~Canvas();

    Fractal::Point pixel_to_set_tlo(int x, int y) const;

    virtual bool on_button_press_event(GdkEventButton * evt);
    virtual bool on_button_release_event(GdkEventButton * evt);
    virtual bool on_motion_notify_event(GdkEventMotion * evt);
    virtual bool on_expose_event(GdkEventExpose * evt);
    virtual bool on_configure_event(GdkEventConfigure * evt);
    virtual bool on_scroll_event(GdkEventScroll * evt);
};

#endif /* CANVAS_H_ */
