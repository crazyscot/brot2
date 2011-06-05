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

bool Canvas::on_button_press_event(GdkEventButton * UNUSED(evt)) {
#if 0 //XXX
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	if (ctx->canvas != NULL) {
		Point new_ctr = pixel_to_set_tlo(ctx->mainctx, event->x, event->y);

		if (event->button >= 1 && event->button <= 3) {
			ctx->mainctx->centre = new_ctr;
			switch(event->button) {
			case 1:
				// LEFT: zoom in a bit
				do_zoom(ctx, ZOOM_IN);
				return TRUE;
			case 3:
				// RIGHT: zoom out
				do_zoom(ctx, ZOOM_OUT);
				return TRUE;
			case 2:
				// MIDDLE: simple pan
				do_zoom(ctx, REDRAW_ONLY);
				return TRUE;
			}
		}
		if (event->button==8) {
			// mouse down: store it
			dragrect_origin_x = dragrect_current_x = (int)event->x;
			dragrect_origin_y = dragrect_current_y = (int)event->y;
			dragrect_active = 1;
			return TRUE;
		}
	}
#endif

	return false;
}
bool Canvas::on_button_release_event(GdkEventButton * UNUSED(evt)) {
#if 0 //XXX
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
		bool silly = false;
		if (event->button != 8)
			return FALSE;

		//printf("button %d UP @ %d,%d\n", event->button, (int)event->x, (int)event->y);

		if (ctx->canvas != NULL) {
			int l = MIN(event->x, dragrect_origin_x);
			int r = MAX(event->x, dragrect_origin_x);
			int t = MAX(event->y, dragrect_origin_y);
			int b = MIN(event->y, dragrect_origin_y);

			if (abs(l-r)<4 || abs(t-b)<4) silly=true;

			dragrect_active = 0;

			if (silly) {
				recolour(ctx->window, ctx); // Repaint over the dragrect
			} else {
				Point TR = pixel_to_set_tlo(ctx->mainctx, r, t);
				Point BL = pixel_to_set_tlo(ctx->mainctx, l, b);
				ctx->mainctx->centre = (TR+BL)/(Value)2.0;
				ctx->mainctx->size = TR - BL;

				do_plot(ctx->window, ctx);
			}
		}
		return TRUE;
#endif
	return false;
}
bool Canvas::on_motion_notify_event(GdkEventMotion * UNUSED(evt)) {
#if 0 //XXX
	_gtk_ctx * ctx = (_gtk_ctx*) _dat;
	int x, y;
	GdkModifierType state;

	if (!dragrect_active) return FALSE;

	if (event->is_hint) {
		gdk_window_get_pointer (event->window, &x, &y, &state);
	} else {
		x = event->x;
		y = event->y;
		state = GdkModifierType(event->state);
	}

	if (!ctx->dragrect)
		ctx->dragrect = gdk_window_create_similar_surface(ctx->window->window,
				CAIRO_CONTENT_COLOR_ALPHA,
				ctx->mainctx->rwidth, ctx->mainctx->rheight);

	cairo_t *cairo = cairo_create(ctx->dragrect);
	clear_dragrect(cairo);
	draw_dragrect(cairo,
			dragrect_origin_x, dragrect_origin_y,
			x - dragrect_origin_x, y - dragrect_origin_y);
	cairo_destroy(cairo);

	dragrect_current_x = x;
	dragrect_current_y = y;

	gtk_widget_queue_draw (widget);

	//printf("button %d @ %d,%d\n", state, x, y);
	return TRUE;
#endif
	return false;
}
bool Canvas::on_expose_event(GdkEventExpose * evt) {
	Glib::RefPtr<Gdk::Window> window = get_window();
	if (!window) return false; // no window yet?
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();

	if (!surface) return true; // Haven't rendered yet? Nothing we can do
#if 0 // TODO Try me
	if (evt) {
		cr->rectangle(evt->area.x, evt->area.y, evt->area.width, evt->area.height);
		cr->clip();
	}
#endif

	cr->set_source(surface, 0, 0);
	cr->paint();

#if 0 //XXX XYZY
	if (dragrect_active && ctx->dragrect) {
		cairo_save(dest);
		cairo_set_source_surface(dest, ctx->dragrect, 0, 0);
		cairo_paint(dest);
		cairo_restore(dest);
	}
	if (ctx->hud) {
		cairo_set_source_surface(dest, ctx->hud, 0, 0);
		cairo_paint(dest);
	}
#endif
	return false;
}

bool Canvas::on_configure_event(GdkEventConfigure * UNUSED(evt)) {
	// XXX do_resize(widget, ctx, event->width, event->height); // XYZY FIXME PRIO
	return TRUE;
}

// XXX XYZY PRIO: bring in render_cairo().
// XXX XYZY PRIO: dragrect
// XXX XYZY PRIO: hud
