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

#include "png.h" // must be first, see launchpad 218409
#include "MainWindow.h"
#include "Menus.h"
#include "Canvas.h"
#include "HUD.h"
#include "libbrot2/Render2.h"
#include "libbrot2/Plot3Plot.h"
#include "misc.h"
#include "config.h"
#include "ControlsWindow.h"
#include "SaveAsPNG.h"
#include "Exception.h"

#include <stdlib.h>
#include <complex>
#include <math.h>
#include <iostream>
#include <queue>

#include <gtkmm/main.h>
#include <gdk/gdkkeysyms.h>
#include <pangomm.h>

using namespace Plot3;
using namespace std;
using namespace Render2;

const double MainWindow::ZOOM_FACTOR = 2.0f;

static inline void surface_error_check(MainWindow& mw, Cairo::RefPtr<Cairo::ImageSurface>* s)
{
	Cairo::ErrorStatus st = (*s)->get_status();
	if (st != 0) {
		std::cerr << "Surface error: " << cairo_status_to_string(st) << std::endl;
		ostringstream ss;
		ss << "Cairo reported an error (" << cairo_status_to_string(st) << "). This isn't good.";
		Util::alert(&mw, ss.str());
	}
}

MainWindow::MainWindow() : Gtk::Window(),
			hud(*this, prefs()),
			controlsWin(*this, prefs()),
			movieWin(*this, prefs()),
			imgbuf(0), plot(0), plot_prev(0), renderer(0),
			rwidth(0), rheight(0),
			draw_hud(true), antialias(false),
			initializing(true),
			aspectfix(false), at_max_zoom(false), at_min_zoom(false), recolour_when_done(false),
			divider(new Plot3::ChunkDivider::SuperpixelVariable(prefs())),
            _chunks_this_pass(0),
			dragrect(*this),
			_threadpool(BrotPrefs::threadpool_size(prefs()))
{
	set_title(PACKAGE_NAME); // Renderer will update this
	vbox = Gtk::manage(new Gtk::VBox());
	vbox->set_border_width(1);
	add(*vbox);

	std::string init_fract = "Mandelbrot",
		init_pal = "Linear rainbow";

	{
		// Sort out the initial plot params.
		// (One day this might become a CLI option?)
		Fractal::FractalCommon::load_base();
		Fractal::FractalImpl* impl = Fractal::FractalCommon::registry.get(init_fract);
		if (impl) {
			size = { impl->xmax - impl->xmin,
					 impl->ymax - impl->ymin };
			centre = { impl->xmin, impl->ymin };
			centre += size/2.0;
		} else {
			// fallback, shouldn't happen but do something vaguely sensible anyway
			centre = {0.0,0.0};
			size = {4.5,4.5};
		}
	}

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
	menubar->optionsMenu->set_controls_status( prefs()->get(PREF(ShowControls)) );

	// Cleanup event(currently only used for SaveAsPNG jobs).
	Glib::signal_timeout().connect( sigc::mem_fun(*this, &MainWindow::on_timer), 500 );
}

MainWindow::~MainWindow() {
	// vbox etc are managed so auto-delete
	if (plot) {
		plot->stop();
		plot->wait();
	}
	delete plot;
	delete plot_prev;
	delete divider;
	delete renderer;
	delete[] imgbuf;
}

void MainWindow::zoom_mechanics(enum Zoom type) {
	switch (type) {
	case ZOOM_IN:
		size /= ZOOM_FACTOR;
		at_min_zoom = at_max_zoom = false;
		{
			/* Don't allow the user to zoom into somewhere we can't do an
			 * antialiased plot.
			 * I found that a limit of 2.0*rwidth*smallest_min didn't work,
			 * presumably this is another precision issue. -wry */
			Fractal::Value d = fabsl(real(size)),
					limit = 2.1 * rwidth * Fractal::Maths::smallest_min_pixel_size();
			if (d < limit) {
				size.real(limit);
				at_max_zoom = true;
			}
			d = fabsl(imag(size));
			limit = 2.1 * rheight * Fractal::Maths::smallest_min_pixel_size();
			if (d < limit) {
				size.imag(limit);
				at_max_zoom = true;
			}
		}
		break;
	case ZOOM_OUT:
		size *= ZOOM_FACTOR;
		at_max_zoom = at_min_zoom = false;
		{
			Fractal::Value d = real(size), lim = fractal->xmax - fractal->xmin;
			if (d > lim) {
				size.real(lim);
				at_min_zoom = true;
			}
			d = imag(size), lim = fractal->ymax - fractal->ymin;
			if (d > lim) {
				size.imag(lim);
				at_min_zoom = true;
			}
		}
		break;
	case REDRAW_ONLY:
		break;
	}
}

void MainWindow::do_zoom(enum Zoom type) {
	if (!canvas) return;
	// LP#1033910: Don't allow a straight zoom in when we're at max.
	if ( (is_at_max_zoom() && type == ZOOM_IN) || (is_at_min_zoom() && type == ZOOM_OUT) )
		return;
	zoom_mechanics(type);
	do_plot(false);
}

void MainWindow::do_zoom(enum Zoom type, const Fractal::Point& newcentre) {
	if (!canvas) return;
	// LP#1033910: Go ahead with a recentring zoom if at max, as we need to replot anyway
	// (but won't actually zoom).
	zoom_mechanics(type);
	new_centre_checked(newcentre, true);
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
	Gtk::Main::instance()->quit(); // NORETURN
	return true;
}

void MainWindow::render_prep(int local_inf) {
	const Cairo::Format FORMAT = Cairo::Format::FORMAT_RGB24;
	const int rowstride = Cairo::ImageSurface::format_stride_for_width(FORMAT, rwidth);

	if (!imgbuf) {
		imgbuf = new guchar[rowstride * rheight];
		// If not zeroed, odd artefacts appear when toggling antialias
		std::fill(imgbuf, &imgbuf[rowstride*rheight], 0);
		delete renderer;
		renderer = 0;
	}
	if (!renderer)
		renderer = new Render2::MemoryBuffer(imgbuf, rowstride, rwidth, rheight, antialias, local_inf, FORMAT, *pal);
	else
		renderer->fresh_local_inf(local_inf);

	if (!canvas->surface) {
		canvas->surface = Cairo::ImageSurface::create(imgbuf, FORMAT, rwidth, rheight, rowstride);
		surface_error_check(*this, &canvas->surface);
	}
}

void MainWindow::render_buffer_updated(Plot3Chunk *job) {
	render(-1, false, false, job);
}

void MainWindow::render_buffer_tidyup() {
	render(-1, false, true);
}

static gboolean idle_queue_draw(gpointer data) {
    ((MainWindow*)(data))->queue_draw();
    return FALSE; // i.e. we're done, remove us from the queue
}

void MainWindow::render(int local_inf, bool do_reprocess, bool may_do_hud, Plot3Chunk *job) {
	// TODO: autolock on gctx ? and everything that accessess gctx->(surfaces)?
	if (!canvas->surface)
		return;

	canvas->surface->reference();
	surface_error_check(*this, &canvas->surface);
	ASSERT(canvas->surface->get_data() != NULL);

	if (do_reprocess) {
		renderer->fresh_local_inf(local_inf);
		renderer->fresh_palette(*pal);
		renderer->process(plot->get_chunks__only_after_completion());
	}

	if (may_do_hud && draw_hud)
		hud.draw(plot, rwidth, rheight);

    if (job)
        canvas->surface->mark_dirty(job->_offX, job->_offY, job->_width, job->_height);
    else
        canvas->surface->mark_dirty();
	canvas->surface->unreference();
    gdk_threads_add_idle(idle_queue_draw, this);
}


void MainWindow::do_resize(unsigned width, unsigned height)
{
	safe_stop_plot();

	if ((rwidth != width) ||
			(rheight != height)) {
		// Size has changed!
		rwidth = width;
		rheight = height;
		destroy_image();
		dragrect.resized();

		{
			// It doesn't make sense to allow undo through a size change.
			Plot3Plot *del1 = plot_prev, *del2 = plot;
			plot_prev = 0;
			plot = 0;
			gdk_threads_leave();
			delete del1;
			delete del2;
			gdk_threads_enter();
		}

		do_plot();
	}
}


// (Re)draws us, then sets up to queue an expose event when it's done.
void MainWindow::do_plot(bool is_same_plot)
{
	if (initializing) return;
	safe_stop_plot();

	ASSERT(canvas); // Initialised in MainWindow()

	progbar->set_text("Plotting pass 1...");

	double aspect;

	aspectfix = false;
	aspect = (double)rwidth / rheight;
	if (imag(size) * aspect != real(size)) {
		size.imag(real(size)/aspect);
		aspectfix=true;
	}

	// N.B. This (gtk/gdk lib calls from non-main thread) will not work at all on win32; will need to refactor if I ever port.
	gettimeofday(&plot_tv_start,0);

	Plot3Plot * deleteme = 0;
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

	ASSERT(!plot);
	unsigned pwidth = rwidth,
			pheight = rheight;
	if (antialias) {
		pwidth *= 2;
		pheight *= 2;
	}
	plot = new Plot3::Plot3Plot(get_threadpool(), this, *fractal, *divider, centre, size, pwidth, pheight);

	render_prep(-1);
	if (draw_hud)
		hud.draw(plot, rwidth, rheight);
	try {
		plot->start();
	} catch (BrotException e) {
		progbar->set_text("Plot failed to start!");
		std::cerr << e.detail() << std::endl;
	}
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

void MainWindow::chunk_done(Plot3Chunk* job)
{
	_chunks_this_pass++;
	if (renderer)
		renderer->process(*job);

	float workdone = (float) _chunks_this_pass / plot->chunks_total();
    ASSERT(workdone >= 0.0);
	ASSERT(workdone <= 1.0);
	gdk_threads_enter();
	progbar->set_fraction(workdone);
	if (renderer)
		render_buffer_updated(job);
	gdk_threads_leave();
}

void MainWindow::pass_complete(std::string& commentary)
{
	_chunks_this_pass=0;

	render_buffer_tidyup(); // applies the HUD
	gdk_threads_enter();
	progbar->set_text(commentary.c_str());
	gdk_threads_leave();
}

void MainWindow::plot_complete()
{
	gdk_threads_enter();
	real_plot_complete();
	gdk_threads_leave();
}

void MainWindow::real_plot_complete()
{
	struct timeval tv_after, tv_diff;

	if (!plot) return;

	progbar->pulse();
	gettimeofday(&tv_after,0);

	tv_diff = Util::tv_subtract(tv_after, plot_tv_start);
	double timetaken = tv_diff.tv_sec + (tv_diff.tv_usec / 1e6);

	set_title(plot->info(false).c_str());

	std::ostringstream info;
	info.precision(4);
	info << timetaken << "s; ";
	info << plot->get_passes() <<" passes; maxiter=" << plot->get_maxiter() << ".";
	if (aspectfix)
		info << " Aspect ratio autofixed.";
	if (at_max_zoom)
		info << " Resolution limit reached, cannot zoom further!";
	if (at_min_zoom)
		info << " Cannot zoom out further!";

	progbar->set_fraction(1.0);
	progbar->set_text(info.str().c_str());

	if (recolour_when_done) // If the palette was changed during the computation.
		render(-1, true, true);

	queue_draw();
	gdk_flush();
}

void MainWindow::recolour()
{
	if (initializing) return;
	if (plot->is_running()) {
		renderer->fresh_palette(*pal);
		recolour_when_done = true;
	}
	else
		render(-1, true, true);
}

void MainWindow::do_undo()
{
	if (!canvas) return;
	if (!plot_prev) {
		progbar->set_text("Nothing to undo");
		return;
	}
	safe_stop_plot();

	Plot3Plot *tmp = plot;
	plot = plot_prev;
	plot_prev = tmp;

	centre = plot->centre;
	size = plot->size;
	rwidth = plot->width;
	rheight = plot->height;
    if (antialias) {
        rwidth /= 2;
        rheight /= 2;
    }

	recolour();
	//progbar->set_text(""); // For some reason this breaks the undo. Boggle.
	progbar->set_fraction(0.0);
}

void MainWindow::do_stop()
{
	progbar->set_text("Stopping at end of this pass...");
	//safe_stop_plot(); // Blocking!
	if (plot) plot->stop(); // Safe enough - other actions will call safe_stop_plot before proceeding.
}

void MainWindow::do_more_iters()
{
	if (!canvas || !plot) {
		std::cerr << "ALERT: do_more called in invalid state - trace me!" << std::endl;
		for (;;) sleep(1);
	}
	if (plot->is_running()) {
		progbar->set_text("Plot already running");
		return;
	}
	gettimeofday(&plot_tv_start,0);
	plot->start();
}

void MainWindow::do_reset()
{
	safe_stop_plot();
	size = { plot->fract.xmax - plot->fract.xmin,
		plot->fract.ymax - plot->fract.ymin };
	centre = { plot->fract.xmin, plot->fract.ymin };
	centre += size/2.0;
	do_plot(false);
}

void MainWindow::update_params(Fractal::Point& ncentre, Fractal::Point& nsize)
{
	size = nsize;
	new_centre_checked(ncentre, false);
}

void MainWindow::new_centre_checked(const Fractal::Point& ncentre, bool is_zoom)
{
	centre = ncentre;

	if (!is_zoom)
		at_min_zoom = at_max_zoom = false;
	// if is_zoom, those mechanics check and update the min/max flags.

	if (real(centre) > fractal->xmax) {
		Fractal::Point shift(fractal->xmax-real(centre), 0);
		centre += shift;
	}
	if (real(centre) < fractal->xmin) {
		Fractal::Point shift(fractal->xmin-real(centre), 0);
		centre += shift;
	}

	if (imag(centre) > fractal->ymax) {
		Fractal::Point shift(0, fractal->ymax-imag(centre));
		centre += shift;
	}
	if (imag(centre) < fractal->ymin) {
		Fractal::Point shift(0, fractal->ymin-imag(centre));
		centre += shift;
	}
}

void MainWindow::toggle_hud()
{
	if (initializing) return;
	draw_hud = !draw_hud;
	recolour();
}

void MainWindow::destroy_image()
{
	safe_stop_plot(); // Paranoia
	if (canvas->surface) {
		canvas->surface->finish();
		canvas->surface.clear();
	}
	delete[] imgbuf;
	imgbuf=0;
}

void MainWindow::toggle_antialias()
{
	if (initializing) return;
	safe_stop_plot();
	antialias = !antialias;
	destroy_image();
    {
        // It doesn't make sense to allow undo through an antialias toggle.
        Plot3Plot *del1 = plot_prev, *del2 = plot;
        plot_prev = 0;
        plot = 0;
        gdk_threads_leave();
        delete del1;
        delete del2;
        gdk_threads_enter();
    }
	do_plot(false);
}

void MainWindow::toggle_fullscreen()
{
	fullscreen_requested = !fullscreen_requested;
	if (fullscreen_requested)
		fullscreen();
	else
		unfullscreen();
}

static std::queue<std::shared_ptr<SavePNG::Single>> png_q; // protected by gdk threads lock.

void MainWindow::queue_png(std::shared_ptr<SavePNG::Single> png)
{
	gdk_threads_enter();
	png_q.push(png);
	gdk_threads_leave();
}

void MainWindow::png_save_completion()
{
	// must have the gdk threads lock.
	if (!png_q.empty()) {
		std::shared_ptr<SavePNG::Single> png(png_q.front());
		png_q.pop();
		//gdk_threads_leave(); // Don't do this, it deadlocks.
		Plot3Plot& pngplot = png->plot;
		pngplot.wait();
		png->instance_to_png(this);
		//gdk_threads_enter();
	} else {
	}
}

bool MainWindow::on_timer()
{
	png_save_completion();
	return true;
}

ThreadPool& MainWindow::get_threadpool()
{
	return _threadpool;
}

unsigned MainWindow::get_menubar_height() {
	if (!menubar)
		return 0;
	return menubar->get_height();
}
