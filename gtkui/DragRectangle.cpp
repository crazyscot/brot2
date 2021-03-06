/*
    DragRectangle: mouse-driven drag-out rectangle graphics/tracking
    Copyright (C) 2010-2011 Ross Younger

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

#include "png.h" // must be first, see launchpad 218409
#include "DragRectangle.h"
#include "MainWindow.h"
#include "Exception.h"

const std::valarray<double> DragRectangle::dashes = { 5.0, 5.0 };

DragRectangle::DragRectangle(MainWindow& window) : parent(window), active(false) {
}

void DragRectangle::resized() {
	if (surface)
		surface->finish();
	surface.clear();
	ASSERT(!surface);
}

void DragRectangle::activate(int x, int y) {
	active = true;
	origin.reinit(x,y);
	current = origin;
}

void DragRectangle::draw() {
	active = false;
	if (current_cairo)
		current_cairo.clear();
	invalidate_dashed(origin, current);
}

void DragRectangle::invalidate_dashed(Util::xy &origin, Util::xy &other) {
	int x1,x2, y1,y2;
	if (origin.x > other.x) {
		x1 = other.x;
		x2 = origin.x;
	} else {
		x1 = origin.x;
		x2 = other.x;
	}
	if (origin.y > other.y) {
		y1 = other.y;
		y2 = origin.y;
	} else {
		y1 = origin.y;
		y2 = other.y;
	}
	/* These given points are relative to the main widget but we need to compensate for the menubar */
	y1 += parent.get_menubar_height();
	y2 += parent.get_menubar_height();

	parent.queue_draw_area(x1, y1, 2+x2-x1, 2);
	parent.queue_draw_area(x2, y1, 2, 2+y2-y1);
	parent.queue_draw_area(x1, y1, 2, 2+y2-y1);
	parent.queue_draw_area(x1, y2, 2+x2-x1, 2);
}

void DragRectangle::draw(int x, int y) {
	Util::xy previous(current);
	current.reinit(x,y);

	if (!surface) {
		Glib::RefPtr<Gdk::Window> w = parent.get_window();
		surface = w->create_similar_surface(Cairo::CONTENT_COLOR_ALPHA, parent.get_rwidth(), parent.get_rheight());
	}

	ASSERT(active);
	{
		if (!current_cairo)
			current_cairo = Cairo::Context::create(surface);

		Cairo::RefPtr<Cairo::Context> cr (current_cairo);

		this->clear();
		invalidate_dashed(origin, previous); // This is OK if we're in a new draw situation, previous is reinitialised to origin by activate().

		int x = origin.x, y = origin.y,
			width = current.x - origin.x, height = current.y - origin.y;

		if (width<0) {
			x+= width;
			width = -width;
		}
		if (height<0) {
			y += height;
			height = -height;
		}
		cr->save();
		cr->set_operator(Cairo::Operator::OPERATOR_OVER);
		cr->rectangle(x,y,width,height);
		cr->set_line_width(2);

		// First do it in black...
		cr->set_source_rgb(0,0,0);
		cr->set_dash(DragRectangle::dashes, 0.0);
		cr->stroke();

		// ... then overlay with white dashes. The call back to cairo_rectangle
		// seems necessary for the overlay to not throw away what we just did.
		cr->rectangle(x,y,width,height);
		cr->set_source_rgb(1,1,1);
		cr->set_dash(DragRectangle::dashes, 5.0);
		cr->stroke();

		cr->restore();
		invalidate_dashed(origin, current);
	}
}

void DragRectangle::clear() {
	Cairo::RefPtr<Cairo::Context> cr (current_cairo);
	cr->save();
	cr->set_source_rgba(0,0,0,0);
	cr->set_operator(Cairo::Operator::OPERATOR_SOURCE);
	cr->paint();
	cr->restore();
}
