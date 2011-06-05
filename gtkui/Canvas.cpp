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

#include "Canvas.h"
#include "MainWindow.h"
#include "HUD.h"
#include "misc.h"

#include <gdkmm/event.h>
#include <cairomm/cairomm.h>

Canvas::Canvas(MainWindow *parent) : main(parent), surface(0) {
	set_size_request(300,300); // XXX default size
	add_events (Gdk::EXPOSURE_MASK
			| Gdk::LEAVE_NOTIFY_MASK
			| Gdk::BUTTON_PRESS_MASK
			| Gdk::BUTTON_RELEASE_MASK
			| Gdk::POINTER_MOTION_MASK
			| Gdk::POINTER_MOTION_HINT_MASK);
}

Canvas::~Canvas() {
}

// Converts a clicked pixel into a fractal Point, origin = top left
Fractal::Point Canvas::pixel_to_set_tlo(int x, int y) const
{
	if (main->antialias) {
		// scale up our click to the plot point within
		x *= main->antialias_factor;
		y *= main->antialias_factor;
	}
	return main->plot->pixel_to_set_tlo(x,y);
}

bool Canvas::on_button_press_event(GdkEventButton *evt) {
	if (!surface) return false;

	if (evt->button==8) {
		// mouse down: store it
		main->dragrect.activate(evt->x, evt->y);
		return true;
	}
	return false;
}

bool Canvas::on_button_release_event(GdkEventButton *evt) {
	if (!surface) return false;
	Fractal::Point clickpos = pixel_to_set_tlo(evt->x, evt->y);
	switch(evt->button) {
	case 8:
		return end_dragrect(evt);
	case 1:
		// LEFT: zoom in a bit
		main->centre = clickpos;
		main->do_zoom(MainWindow::Zoom::ZOOM_IN);
		return true;
	case 3:
		// RIGHT: zoom out
		main->centre = clickpos;
		main->do_zoom(MainWindow::Zoom::ZOOM_OUT);
		return true;
	case 2:
		// MIDDLE: simple pan
		main->centre = clickpos;
		main->do_zoom(MainWindow::Zoom::REDRAW_ONLY);
		return true;
	}
	return false;
}

bool Canvas::end_dragrect(GdkEventButton *evt) {
	bool silly = false;

	//printf("button %d UP @ %d,%d\n", event->button, (int)event->x, (int)event->y);

	int l = MIN(evt->x, main->dragrect.get_origin().x);
	int r = MAX(evt->x, main->dragrect.get_origin().x);
	int t = MAX(evt->y, main->dragrect.get_origin().y);
	int b = MIN(evt->y, main->dragrect.get_origin().y);

	if (abs(l-r)<4 || abs(t-b)<4) silly=true;

	main->dragrect.draw();

	if (silly) {
		main->recolour(); // Just get rid of it
	} else {
		Fractal::Point TR = pixel_to_set_tlo(r, t);
		Fractal::Point BL = pixel_to_set_tlo(l, b);
		main->centre = (TR+BL)/(Fractal::Value)2.0;
		main->size = TR - BL;
		main->do_plot(false);
	}
	return true;
}

bool Canvas::on_motion_notify_event(GdkEventMotion * UNUSED(evt)) {
	if (!surface) return false;
	if (!main->dragrect.is_active()) return false;
	int x, y;
	get_pointer(x,y);
	main->dragrect.draw(x,y);
	return true;
}

bool Canvas::on_expose_event(GdkEventExpose * evt) {
	Glib::RefPtr<Gdk::Window> window = get_window();
	if (!window) return false; // no window yet?
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();

	if (!surface) return true; // Haven't rendered yet? Nothing we can do
	if (evt) {
#if 0 // TODO Try me
		cr->rectangle(evt->area.x, evt->area.y, evt->area.width, evt->area.height);
		cr->clip();
#endif
	}

	cr->set_source(surface, 0, 0);
	cr->paint();

	if (main->dragrect.is_active()) {
		cr->save();
		cr->set_source(main->dragrect.get_surface(), 0, 0);
		cr->paint();
		cr->restore();
	}

	cr->set_source(main->hud.get_surface(), 0, 0); // TODO HUD position
	cr->paint();
	return true;
}

bool Canvas::on_configure_event(GdkEventConfigure *evt) {
	main->do_resize(evt->width, evt->height);
	return true;
}
