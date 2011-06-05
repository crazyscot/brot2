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
#include "HUD.h"
#include "misc.h"
#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <complex>
#include <math.h>
#include <iostream>

#include <gtkmm/main.h>
#include <gdk/gdkkeysyms.h>
#include <pangomm.h>

const int MainWindow::DEFAULT_ANTIALIAS_FACTOR = 2;

MainWindow::MainWindow() : Gtk::Window(),
			imgbuf(0), plot(0), plot_prev(0),
			rwidth(0), rheight(0),
			draw_hud(true), antialias(false),
			antialias_factor(DEFAULT_ANTIALIAS_FACTOR), initializing(true),
			aspectfix(false), clip(false) {
	set_title(PACKAGE_NAME); // Renderer will update this
	vbox = Gtk::manage(new Gtk::VBox());
	vbox->set_border_width(1);
	add(*vbox);

	// Default plot params:
	centre = {0.0,0.0};
	size = {4.5,4.5};

	menubar = Gtk::manage(new menus::Menus());
	{
		// XXX TEMP until menus in place:
		fractal = Fractal::FractalRegistry::registry()["Mandelbrot"];
		pal = SmoothPalette::registry["Linear rainbow"];
		assert(fractal);
		assert(pal);
	}
	vbox->pack_start(*menubar, false, false, 0);

	canvas = Gtk::manage(new Canvas(this));
	vbox->pack_start(*canvas, true, true, 0);

	progbar = Gtk::manage(new Gtk::ProgressBar());
	progbar->set_text("Initialising");
	progbar->set_ellipsize(Pango::ELLIPSIZE_END);
	progbar->set_pulse_step(0.1);
	vbox->pack_end(*progbar, false, false, 0);

	hud = new HUD(*this);

	// _main_ctx.pal initial setting by setup_colour_menu().
	// render_ctx.fractal set by setup_fractal_menu().
}

MainWindow::~MainWindow() {
	// vbox etc are managed so auto-delete
	delete plot;
	delete plot_prev;
	delete imgbuf;
	delete hud;
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


void MainWindow::render_cairo(int local_inf) {
	const Cairo::Format FORMAT = Cairo::Format::FORMAT_RGB24;
	const int rowstride = Cairo::ImageSurface::format_stride_for_width(FORMAT, rwidth);

	if (!imgbuf)
		imgbuf = new guchar[rowstride * rheight];

	if (!canvas->surface) {
		canvas->surface = Cairo::ImageSurface::create(imgbuf, FORMAT, rwidth, rheight, rowstride);
		Cairo::ErrorStatus st = canvas->surface->get_status();
		if (st != 0) {
			std::cerr << "Surface error: " << cairo_status_to_string(st) << std::endl;
		}
	}
	canvas->surface->reference(); // TODO: Is this right? It should be created with 1 ref, we then add a 2nd and deref when done.

	// TODO: autolock on gctx ? and everything that accessess gctx->(surfaces)?
	{
		unsigned char *t = canvas->surface->get_data();
		if (!t)
			std::cerr << "Surface data is NULL!" <<std::endl;
		{
			cairo_status_t st = canvas->surface->get_status();
			if (st != 0) {
				std::cerr << "Surface error: " << cairo_status_to_string(st) << std::endl;
			}
		}
	}

	if (!render_generic(canvas->surface->get_data(), rowstride,
			local_inf, FORMAT)) { // Oops, it vanished
		return;
	}

	if (draw_hud)
		hud->draw(plot, rwidth, rheight);

	canvas->surface->mark_dirty();
	canvas->surface->unreference();
}


/*
 * The actual work of turning an array of fractal_points - maybe an antialiased
 * set - into a packed array of pixels to a given format.
 *
 * buf: Where to put the data. This should be at least
 * (rowstride * rctx->height) bytes long.
 *
 * rowstride: the size of an output row, in bytes. In other words the byte
 * offset from one row to the next - which may be different from
 * (bytes per pixel * rctx->rwidth) if any padding is required.
 *
 * local_inf: the local plot's current idea of infinity.
 * (N.B. -1 is always treated as infinity.)
 *
 * fmt: The byte format to use. This may be a CAIRO_FORMAT_* or our
 * internal PACKED_RGB_24 (used for png output).
 *
 * Returns: True if the render completed, false if the plot disappeared under
 * our feet (typically by the user doing something to cause us to render
 * afresh).
 */
// TODO: This function does not belong in MainWindow - does it?
bool MainWindow::render_generic(unsigned char *buf, const int rowstride, const int local_inf, pixpack_format fmt)
{
	assert(buf);
	assert((unsigned)rowstride >= RGB_BYTES_PER_PIXEL * rwidth);

	const Fractal::PointData * data = plot->get_data();
	if (!plot || !data) return false; // Oops, disappeared under our feet

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but gdk assumes a top-left origin.

	const unsigned factor = antialias ? antialias_factor : 1;

	const Fractal::PointData ** srcs = new const Fractal::PointData* [ factor ];
	unsigned i,j;
	for (j=0; j<rheight; j++) {
		guchar *dst = &buf[j*rowstride];
		const unsigned src_idx = (rheight - j - 1) * factor;
		for (unsigned k=0; k < factor; k++) {
			srcs[k] = &data[plot->width * (src_idx+k)];
		}

		for (i=0; i<rwidth; i++) {
			unsigned rr=0, gg=0, bb=0; // Accumulate the result

			for (unsigned k=0; k < factor; k ++) {
				for (unsigned l=0; l < factor; l++) {
					rgb pix1 = render_pixel(&srcs[k][l], local_inf, pal);
					rr += pix1.r; gg += pix1.g; bb += pix1.b;
				}
			}
			switch(fmt) {
				case CAIRO_FORMAT_ARGB32:
				case CAIRO_FORMAT_RGB24:
					// alpha=1.0 so these cases are the same.
					dst[3] = 0xff;
					dst[2] = rr/(factor * factor);
					dst[1] = gg/(factor * factor);
					dst[0] = bb/(factor * factor);
					dst += 4;
					break;

				case pixpack_format::PACKED_RGB_24:
					dst[0] = rr/(factor * factor);
					dst[1] = gg/(factor * factor);
					dst[2] = bb/(factor * factor);
					dst += 3;
					break;

				default:
					abort();
			}
			for (unsigned k=0; k < factor; k++) {
				srcs[k] += factor;
			}
		}
	}
	return true;
}


void MainWindow::do_resize(unsigned width, unsigned height)
{
	safe_stop_plot();

	if ((rwidth != width) ||
			(rheight != height)) {
		// Size has changed!
		rwidth = width;
		rheight = height;
		if (canvas->surface) {
			canvas->surface->finish();
			canvas->surface.clear();
		}
		delete[] imgbuf;
		imgbuf=0;
#if 0 //XXX DRAGRECT
		if (ctx->dragrect) {
			cairo_surface_destroy(ctx->dragrect);
			ctx->dragrect = 0;
		}
#endif
	}
	do_plot();
}


// (Re)draws us, then sets up to queue an expose event when it's done.
void MainWindow::do_plot(bool is_same_plot)
{
	safe_stop_plot();

	if (!canvas) {
		// First time? Grey it all out.
		Glib::RefPtr<Gdk::Window> window = get_window();
		if (!window) return; // no window yet?
		Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
		cr->set_source_rgb(0.5, 0.5, 0.5);
		cr->paint();
	}

	progbar->set_text("Plotting pass 1...");

	double aspect;

	aspect = (double)rwidth / rheight;
	if (imag(size) * aspect != real(size)) {
		size.imag(real(size)/aspect);
		aspectfix=1;
	}
	if (fabs(real(size)/rwidth) < MINIMUM_PIXEL_SIZE) {
		size.real(MINIMUM_PIXEL_SIZE*rwidth);
		clip = 1;
	}
	if (fabs(imag(size)/rheight) < MINIMUM_PIXEL_SIZE) {
		size.imag(MINIMUM_PIXEL_SIZE*rheight);
		clip = 1;
	}

	// N.B. This (gtk/gdk lib calls from non-main thread) will not work at all on win32; will need to refactor if I ever port.
	gettimeofday(&plot_tv_start,0);

	Plot2 * deleteme = 0;
	if (is_same_plot) {
		deleteme = plot;
		plot = 0;
	} else {
		if (plot_prev)
			deleteme = plot_prev;
		plot_prev = plot;
		plot = 0;
	}

	if (deleteme) {
		// Must release the mutex as the thread join may block...
		gdk_threads_leave();
		delete deleteme;
		gdk_threads_enter();
	}

	assert(!plot);
	unsigned pwidth = rwidth,
			pheight = rheight;
	if (antialias) {
		pwidth *= antialias_factor;
		pheight *= antialias_factor;
	}
	plot = new Plot2(fractal, centre, size, pwidth, pheight);
	plot->start(this); // <<<< HERE <<<< callbacks !!
	// TODO try/catch (and in do_resume) - report failure. Is gtkmm exception-safe?
}

void MainWindow::safe_stop_plot() {
	// As it stands this function must only ever be called from the main thread.
	// If this assumption later fails to hold, need to vary it to not
	// twiddle the gdk_threads lock.
	if (plot) {
		gdk_threads_leave();
		plot->stop();
		plot->wait();
		gdk_threads_enter();
	}
}

void MainWindow::plot_progress_minor(Plot2& UNUSED(plot), float workdone) {
	gdk_threads_enter();
	progbar->set_fraction(workdone);
	gdk_threads_leave();
}

void MainWindow::plot_progress_major(Plot2& UNUSED(plot), unsigned current_maxiter, std::string& commentary) {
	render_cairo(current_maxiter);
	gdk_threads_enter();
	progbar->set_fraction(0.98);
	progbar->set_text(commentary.c_str());
	queue_draw();
	gdk_threads_leave();

}

void MainWindow::plot_progress_complete(Plot2& plot) {
	struct timeval tv_after, tv_diff;

	gdk_threads_enter();
	progbar->pulse();
	recolour();
	gettimeofday(&tv_after,0);

	tv_diff = Util::tv_subtract(tv_after, plot_tv_start);
	double timetaken = tv_diff.tv_sec + (tv_diff.tv_usec / 1e6);

	set_title(plot.info(false).c_str());

	std::ostringstream info;
	info.precision(4);
	info << timetaken << "s; ";
	info << plot.get_passes() <<" passes; maxiter=" << plot.get_maxiter() << ".";
	if (aspectfix)
		info << " Aspect ratio autofixed.";
	if (clip)
		info << " Resolution limit reached, cannot zoom further!";
	clip = aspectfix = false;

	progbar->set_fraction(1.0);
	progbar->set_text(info.str().c_str());

	queue_draw();
	gdk_flush();
	gdk_threads_leave();
}

void MainWindow::recolour()
{
	render_cairo();
	queue_draw();
}
