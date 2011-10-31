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
#include "Render.h"
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
const double MainWindow::ZOOM_FACTOR = 2.0f;

MainWindow::MainWindow() : Gtk::Window(),
			hud(*this),
			imgbuf(0), plot(0), plot_prev(0),
			rwidth(0), rheight(0),
			draw_hud(true), antialias(false),
			antialias_factor(DEFAULT_ANTIALIAS_FACTOR), initializing(true),
			aspectfix(false), clip(false),
			dragrect(*this) {
	set_title(PACKAGE_NAME); // Renderer will update this
	vbox = Gtk::manage(new Gtk::VBox());
	vbox->set_border_width(1);
	add(*vbox);

	// Default plot params:
	centre = {0.0,0.0};
	size = {4.5,4.5};

	std::string init_fract = "Mandelbrot",
		init_pal = "Linear rainbow";

	menubar = Gtk::manage(new menus::Menus(*this, init_fract, init_pal));
	vbox->pack_start(*menubar, false, false, 0);

	canvas = Gtk::manage(new Canvas(this));
	vbox->pack_start(*canvas, true, true, 0);

	progbar = Gtk::manage(new Gtk::ProgressBar());
	progbar->set_text("Initialising");
	progbar->set_ellipsize(Pango::ELLIPSIZE_END);
	progbar->set_pulse_step(0.1);
	vbox->pack_end(*progbar, false, false, 0);

	// _main_ctx.pal initial setting by setup_colour_menu().
	// render_ctx.fractal set by setup_fractal_menu().

	initializing = false;
}

MainWindow::~MainWindow() {
	// vbox etc are managed so auto-delete
	if (plot) {
		plot->stop();
		plot->wait();
	}
	delete plot;
	delete plot_prev;
	delete imgbuf;
}

void MainWindow::zoom_mechanics(enum Zoom type) {
	switch (type) {
	case ZOOM_IN:
		size /= ZOOM_FACTOR;
		break;
	case ZOOM_OUT:
		size *= ZOOM_FACTOR;
		{
			Fractal::Value d = real(size), lim = fractal->xmax - fractal->xmin;
			if (d > lim)
				size.real(lim);
			d = imag(size), lim = fractal->ymax - fractal->ymin;
			if (d > lim)
				size.imag(lim);
		}
		break;
	case REDRAW_ONLY:
		break;
	}
}

void MainWindow::do_zoom(enum Zoom type) {
	if (!canvas) return;
	zoom_mechanics(type);
	do_plot(false);
}

void MainWindow::do_zoom(enum Zoom type, const Fractal::Point& newcentre) {
	if (!canvas) return;
	zoom_mechanics(type);
	new_centre_checked(newcentre);
	do_plot(false);
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
	safe_stop_plot();
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

	if (!Render::render_generic(canvas->surface->get_data(), rowstride,
			local_inf, Render::pixpack_format(FORMAT), *plot, rwidth, rheight, antialias ? antialias_factor : 1, *pal)) { // Oops, it vanished
		return;
	}

	if (draw_hud)
		hud.draw(plot, rwidth, rheight);

	canvas->surface->mark_dirty();
	canvas->surface->unreference();
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
		dragrect.resized();
	}
	do_plot();
}


// (Re)draws us, then sets up to queue an expose event when it's done.
void MainWindow::do_plot(bool is_same_plot)
{
	if (initializing) return;
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
	plot->start(this);
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
	if (initializing) return;
	render_cairo();
	queue_draw();
}

void MainWindow::do_undo()
{
	if (!canvas) return;
	if (!plot_prev) {
		progbar->set_text("Nothing to undo");
		return;
	}

	Plot2 *tmp = plot;
	plot = plot_prev;
	plot_prev = tmp;

	centre = plot->centre;
	size = plot->size;
	rwidth = plot->width;
	rheight = plot->height;

	recolour();
}

void MainWindow::do_stop()
{
	progbar->set_text("Stopping...");
	safe_stop_plot();
	progbar->set_text("Stopped at user request");
	progbar->set_fraction(0.0);
}

void MainWindow::do_more_iters()
{
	if (!canvas || !plot) {
		std::cerr << "ALERT: do_more called in invalid state - trace me!" << std::endl;
		for (;;) sleep(1);
	}
	if (!plot->is_done()) {
		progbar->set_text("Plot already running");
		return;
	}
	gettimeofday(&plot_tv_start,0);
	plot->start(this, true);
}

void MainWindow::update_params(Fractal::Point& ncentre, Fractal::Point& nsize)
{
	size = nsize;
	new_centre_checked(ncentre);
}

void MainWindow::new_centre_checked(const Fractal::Point& ncentre)
{
	centre = ncentre;
	Fractal::Point halfsize = Fractal::divide(size,2.0);
	Fractal::Point TR = centre + halfsize,
				   BL = centre - halfsize;
	bool clipped = false;

	if (real(TR) > fractal->xmax) {
		Fractal::Point shift(fractal->xmax-real(TR), 0);
		centre += shift; TR += shift; BL += shift;
	}
	if (real(BL) < fractal->xmin) {
		Fractal::Point shift(fractal->xmin-real(BL), 0);
		centre += shift; TR += shift; BL += shift;
		if (real(TR) > fractal->xmax) {
			// I'm not sure how this might come about, but my sixth sense tells me to cope with it anyway.
			TR.real(fractal->xmax);
			clipped = true;
		}
	}

	if (imag(TR) > fractal->ymax) {
		Fractal::Point shift(0, fractal->ymax-imag(TR));
		centre += shift; TR += shift; BL += shift;
	}
	if (imag(BL) < fractal->ymin) {
		Fractal::Point shift(0, fractal->ymin-imag(BL));
		centre += shift; TR += shift; BL += shift;
		if (imag(TR) > fractal->ymax) {
			// I'm not sure how this might come about, but my sixth sense tells me to cope with it anyway.
			TR.imag(fractal->ymax);
			clipped = true;
		}
	}
	if (clipped) {
		// need to recompute size and centre.
		size = TR - BL;
		centre = TR - Fractal::divide(size,2.0);
	}
}

void MainWindow::toggle_hud()
{
	if (initializing) return;
	draw_hud = !draw_hud;
	recolour();
}

void MainWindow::toggle_antialias()
{
	if (initializing) return;
	safe_stop_plot();
	antialias = !antialias;
	do_plot(false);
}
