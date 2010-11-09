/*
    brot2/main.cpp: Yet Another Mandelbrot Plotter (main program)
    Copyright (C) 2010 Ross Younger

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

#define _GNU_SOURCE 1
#include <gtk/gtk.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <complex>
#include <pthread.h>

#include "Plot.h"
#include "Fractal.h"
#include "palette.h"

using namespace std;

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

typedef struct _render_ctx {
	Plot * plot;
	DiscretePalette * pal;
	// Yes, the following are mostly the same as in the Plot - but the plot is torn down and recreated frequently.
	Fractal *fractal;
	cdbl centre, size;
	unsigned width, height, maxiter;

	_render_ctx(): plot(0), pal(0), fractal(0) {};
} _render_ctx;

typedef struct _gtk_ctx {
	_render_ctx * mainctx;
	GtkWidget *window;
	GdkPixmap *render;
	GtkStatusbar *statusbar;
	GtkWidget * colour_menu;
	GSList * colours_radio_group;

	pthread_mutex_t render_lock; // lock before starting a render, unlock when complete
	GThread * t_render_main;

	_gtk_ctx() : mainctx(0), window(0), render(0), statusbar(0), colour_menu(0), colours_radio_group(0), t_render_main(0) {
		pthread_mutex_init(&render_lock, 0);
	};
} _gtk_ctx;

static void do_redraw(GtkWidget *widget, _gtk_ctx *ctx);

static void render_gdk(GdkPixmap *dest, GdkGC *gc, _render_ctx & ctx) {
	const gint rowbytes = ctx.width * 3;
	const gint rowstride = rowbytes + 8-(rowbytes%8);
	guchar *buf = new guchar[rowstride * ctx.height]; // packed 24-bit data

	const fractal_point * data = ctx.plot->get_plot_data();
	assert(data);

	unsigned i,j;
	for (j=0; j<ctx.height; j++) {
		guchar *row = &buf[j*rowstride];
		for (i=0; i<ctx.width; i++) {
			if (data->iter == ctx.maxiter) {
				row[0] = row[1] = row[2] = 0;
			} else {
				rgb& col = (*ctx.pal)[data->iter];
				row[0] = col.r;
				row[1] = col.g;
				row[2] = col.b;
			}
			row += 3;
			++data;
		}
	}

	gdk_draw_rgb_image(dest, gc,
			0, 0, ctx.width, ctx.height, GDK_RGB_DITHER_NONE, buf, rowstride);

	delete[] buf;
}

static void update_entry_float(GtkWidget *entry, long double val)
{
	char * tmp = 0;
	if (-1==asprintf(&tmp, "%Lf", val))
		abort(); // gah
	gtk_entry_set_text(GTK_ENTRY(entry), tmp);
	free(tmp);
}

static void read_entry_float(GtkWidget *entry, long double *val_out)
{
	long double tmp;
	const gchar * raw = gtk_entry_get_text(GTK_ENTRY(entry));
	if (1 == sscanf(raw, "%Lf", &tmp)) {
		*val_out = tmp;
		// TODO: Better error handling. Perhaps a validity check at dialog run time?
	}
}

void do_config(gpointer _ctx, guint callback_action, GtkWidget *widget)
//void do_config(GtkWidget *widget, gpointer _ctx, guint callback_action)
{
	_gtk_ctx * ctx = (_gtk_ctx*)_ctx;
	assert (ctx);
	// TODO: generate config dialog from renderer parameters?
	GtkWidget *dlg = gtk_dialog_new_with_buttons("Configuration", GTK_WINDOW(ctx->window),
			GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			NULL);
	GtkWidget * content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	GtkWidget * label;

	//label = gtk_label_new ("Configuration");
	//gtk_container_add (GTK_CONTAINER (content_area), label);

	GtkWidget *c_re, *c_im, *size_re, *size_im;
	c_re = gtk_entry_new();
	update_entry_float(c_re, real(ctx->mainctx->centre));
	c_im = gtk_entry_new();
	update_entry_float(c_im, imag(ctx->mainctx->centre));
	size_re = gtk_entry_new();
	update_entry_float(size_re, real(ctx->mainctx->size));
	size_im = gtk_entry_new();
	update_entry_float(size_im, imag(ctx->mainctx->size));

	GtkWidget * box = gtk_table_new(4, 2, TRUE);

	label = gtk_label_new("Centre Real (x)");
	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(box), c_re, 1, 2, 0, 1);

	label = gtk_label_new("Centre Imaginary (y)");
	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(box), c_im, 1, 2, 1, 2);

	label = gtk_label_new("Size Real (x)");
	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(box), size_re, 1, 2, 2, 3);

	label = gtk_label_new("Size Imaginary (y)");
	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(box), size_im, 1, 2, 3, 4);

	gtk_container_add (GTK_CONTAINER (content_area), box);

	gtk_widget_show_all(dlg);
	gint result = gtk_dialog_run(GTK_DIALOG(dlg));
	if (result == GTK_RESPONSE_ACCEPT) {
		long double res=0;
		read_entry_float(c_re, &res);
		ctx->mainctx->centre.real(res);
		read_entry_float(c_im, &res);
		ctx->mainctx->centre.imag(res);
		read_entry_float(size_re, &res);
		ctx->mainctx->size.real(res);
		read_entry_float(size_im, &res);
		ctx->mainctx->size.imag(res);
		do_redraw(ctx->window, ctx);
	}
	gtk_widget_destroy(dlg);
}

struct timeval tv_subtract (struct timeval tv1, struct timeval tv2)
{
	struct timeval rv;
	rv.tv_sec = tv1.tv_sec - tv2.tv_sec;
	if (tv1.tv_usec < tv2.tv_usec) {
		rv.tv_usec = tv1.tv_usec + 1e6 - tv2.tv_usec;
		--rv.tv_sec;
	} else {
		rv.tv_usec = tv1.tv_usec - tv2.tv_usec;
	}

	return rv;
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	return FALSE;
}

static void destroy_event(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}


/* Returns a menubar widget made from the above menu */
static GtkWidget *make_menubar( GtkWidget  *window, GtkItemFactoryEntry* menu_items, gint nmenu_items, _gtk_ctx * ctx)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	/* Make an accelerator group (shortcut keys) */
	accel_group = gtk_accel_group_new ();

	/* Make an ItemFactory (that makes a menubar) */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
			accel_group);

	/* This function generates the menu items. Pass the item factory,
	   the number of items in the array, the array itself, and any
	   callback data for the the menu items. */
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, ctx);

	/* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}


static void recolour(GtkWidget * widget, _gtk_ctx *ctx)
{
	render_gdk(ctx->render, widget->style->white_gc, *ctx->mainctx);
	gtk_widget_queue_draw(widget);
}

/* per sub-thread context/info */
typedef struct _thread_ctx {
	GThread * thread;
	_render_ctx * rdr;
	unsigned firstrow, nrows;
	_thread_ctx() : thread(0), rdr(0) {};
} _thread_ctx;

static gpointer render_sub_thread(gpointer arg)
{
	_thread_ctx * tctx = (_thread_ctx*) arg;
	tctx->rdr->plot->do_some(tctx->firstrow, tctx->nrows);
	return 0;
}

/*
 * statusbar contexts:
 * 0 = general info
 * 1 = semi-alert e.g. "drawing already in progress"
 */

static gpointer main_render_thread(gpointer arg)
{
	int i;
	struct timeval tv_start, tv_after, tv_diff;
	_gtk_ctx *ctx = (_gtk_ctx*)arg;

	// N.B. This (gtk/gdk lib calls from non-main thread) will not work at all on win32; will need to refactor if I ever port.
	gettimeofday(&tv_start,0);

	if (ctx->mainctx->plot) delete ctx->mainctx->plot;

	// TODO: preference for how many threads
#define NTHREADS 2
	_thread_ctx threads[NTHREADS];

	ctx->mainctx->plot = new Plot(ctx->mainctx->fractal, ctx->mainctx->centre, ctx->mainctx->size, ctx->mainctx->maxiter, ctx->mainctx->width, ctx->mainctx->height);
	ctx->mainctx->plot->prepare();
	const unsigned step = ctx->mainctx->height / NTHREADS;
	GError * err = 0;

	for (i=0; i<NTHREADS; i++) {
		threads[i].rdr = ctx->mainctx;
		threads[i].firstrow = step*i;
		threads[i].nrows = step;
		threads[i].thread = g_thread_create(render_sub_thread, &threads[i], TRUE, &err);
		if (!threads[i].thread) {
			fprintf(stderr, "Could not spawn render thread %d: %d: %s\n", i, err->code, err->message);
			abort();
			// TODO: gtk error message? this is pretty dire though.
		}
	}

	for (i=0; i<NTHREADS; i++)
		g_thread_join(threads[i].thread); // ignore rv.

	// And now turn it into an RGB.
	gdk_threads_enter();
	recolour(ctx->window,ctx);
	// TODO convert to cairo.
	gettimeofday(&tv_after,0);


	tv_diff = tv_subtract(tv_after, tv_start);
	double timetaken = tv_diff.tv_sec + (tv_diff.tv_usec / 1e6);

	gtk_statusbar_pop(ctx->statusbar, 0);
	std::ostringstream info;
	info << ctx->mainctx->plot->info_short();

	std::cout << info.str() << std::endl; // TODO: TEMP
	gtk_window_set_title(GTK_WINDOW(ctx->window), info.str().c_str());

	info.str("");
	info << "rendered in " << timetaken << "s";
	std::cout << info.str() << std::endl; // TODO: TEMP
	gtk_statusbar_push(ctx->statusbar, 0, info.str().c_str());
	gtk_statusbar_pop(ctx->statusbar, 1);

	gtk_widget_queue_draw(ctx->window);
	gdk_flush();
	gdk_threads_leave();

	pthread_mutex_unlock(&ctx->render_lock);

	return 0;
}


// Redraws us onto a given widget (window), then queues an expose event
static void do_redraw(GtkWidget *widget, _gtk_ctx *ctx)
{
	if (0 != pthread_mutex_trylock(&ctx->render_lock)) {
		gtk_statusbar_push(ctx->statusbar, 1, "Plot already in progress");
		return;
	}

	if ((ctx->mainctx->width != (unsigned)widget->allocation.width) ||
			(ctx->mainctx->height != (unsigned)widget->allocation.height)) {
		// Size has changed!
		ctx->mainctx->width = widget->allocation.width;
		ctx->mainctx->height = widget->allocation.height;
		if (ctx->render) {
			g_object_unref(ctx->render);
			ctx->render = 0;
		}
	}

	if (!ctx->render) {
		ctx->render = gdk_pixmap_new(widget->window,
				ctx->mainctx->width,
				ctx->mainctx->height,
				-1);
		// mid_gc[0] gives us grey.
		gdk_draw_rectangle(ctx->render, widget->style->mid_gc[0], true, 0, 0, ctx->mainctx->width, ctx->mainctx->height);
	}

	gtk_statusbar_pop(ctx->statusbar, 0);
	gtk_statusbar_push(ctx->statusbar, 0, "Plotting...");

	GError * err = 0;
	ctx->t_render_main = g_thread_create(main_render_thread, ctx, FALSE, &err);
	// Not joinable, i.e. when it's done, it's done.
	if (!ctx->t_render_main) {
		fprintf(stderr, "Could not spawn thread: %d: %s\n", err->code, err->message);
		abort();
		// TODO: gtk error message? this is pretty dire though.
	}


}

static void colour_menu_selection(_gtk_ctx *ctx, DiscretePalette *sel)
{
	ctx->mainctx->pal = sel;
	if (ctx->mainctx->plot)
		recolour(ctx->window, ctx);
}

static void colour_menu_selection1(gpointer *dat, GtkMenuItem *mi)
{
	const char * lbl = gtk_menu_item_get_label(mi);
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	DiscretePalette * sel = DiscretePalette::registry[lbl];
	colour_menu_selection(ctx, sel);
}


static void setup_colour_menu(_gtk_ctx *ctx, GtkWidget *menubar, DiscretePalette *initial)
{
	ctx->colour_menu = gtk_menu_new();

	std::map<std::string,DiscretePalette*>::iterator it;
	for (it = DiscretePalette::registry.begin(); it != DiscretePalette::registry.end(); it++) {
		GtkWidget * item = gtk_radio_menu_item_new_with_label(ctx->colours_radio_group, it->first.c_str());
		ctx->colours_radio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

		gtk_menu_append(GTK_MENU(ctx->colour_menu), item);
		if (it->second == initial) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		}
		gtk_signal_connect_object(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(colour_menu_selection1), ctx);
		gtk_widget_show(item);
	}
	colour_menu_selection(ctx, initial);

    GtkWidget* colour_item = gtk_menu_item_new_with_mnemonic ("_Colour");
    gtk_widget_show (colour_item);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (colour_item), ctx->colour_menu);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), colour_item);
}

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer *dat)
{
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	do_redraw(widget, ctx);
	return TRUE;
}

static gboolean expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer *dat )
{
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	gdk_draw_drawable(widget->window,
			widget->style->fg_gc[gtk_widget_get_state (widget)],
			ctx->render,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);

	return FALSE;
}

static gint dragrect_origin_x, dragrect_origin_y,
			dragrect_active=0, dragrect_current_x, dragrect_current_y;

static gboolean button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer *dat )
{
#define ZOOM_FACTOR 2.0f
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	if (ctx->render != NULL) {
		int redraw=0;
		cdbl new_ctr = ctx->mainctx->plot->pixel_to_set(event->x, event->y);

		if (event->button >= 1 && event->button <= 3) {
			ctx->mainctx->centre = new_ctr;
			switch(event->button) {
			case 1:
				// LEFT: zoom in a bit
				ctx->mainctx->size /= ZOOM_FACTOR;
				break;
			case 3:
				// RIGHT: zoom out
				ctx->mainctx->size *= ZOOM_FACTOR;
				{
					double d = real(ctx->mainctx->size), lim = ctx->mainctx->fractal->xmax - ctx->mainctx->fractal->xmin;
					if (d > lim)
						ctx->mainctx->size.real(lim);
					d = imag(ctx->mainctx->size), lim = ctx->mainctx->fractal->ymax - ctx->mainctx->fractal->ymin;
					if (d > lim)
						ctx->mainctx->size.imag(lim);
				}
				break;
			case 2:
				// MIDDLE: simple pan
				break;
			}
			redraw = 1;
		}
		if (event->button==8) {
			// mouse down: store it
			dragrect_origin_x = dragrect_current_x = (int)event->x;
			dragrect_origin_y = dragrect_current_y = (int)event->y;
			dragrect_active = 1;
		}
		if (redraw)
			do_redraw(ctx->window, ctx);
	}

	return TRUE;
}

static gboolean button_release_event( GtkWidget *widget, GdkEventButton *event, gpointer *dat )
{
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	if (event->button != 8)
		return FALSE;

	//printf("button %d UP @ %d,%d\n", event->button, (int)event->x, (int)event->y);

	if (ctx->render != NULL) {
		int l = MIN(event->x, dragrect_origin_x);
		int r = MAX(event->x, dragrect_origin_x);
		int t = MIN(event->y, dragrect_origin_y);
		int b = MAX(event->y, dragrect_origin_y);

		// centres
		cdbl TL = ctx->mainctx->plot->pixel_to_set(l,t);
		cdbl BR = ctx->mainctx->plot->pixel_to_set(r,b);
		ctx->mainctx->centre = (TL+BR)/(long double)2.0;
		ctx->mainctx->size = BR - TL;

		dragrect_active = 0;

		do_redraw(ctx->window, ctx);
	}
	return TRUE;
}

static gboolean motion_notify_event( GtkWidget *widget, GdkEventMotion *event, gpointer * _dat)
{
	_gtk_ctx * ctx = (_gtk_ctx*) _dat;
	int x, y;
	GdkModifierType state;

	if (!dragrect_active) return FALSE;

	if (event->is_hint)
		gdk_window_get_pointer (event->window, &x, &y, &state);
	else
	{
		x = event->x;
		y = event->y;
		state = GdkModifierType(event->state);
	}

	gdk_gc_set_function(ctx->window->style->black_gc, GDK_INVERT);

	// turn off previous hilight rect
	gdk_draw_rectangle(ctx->render, widget->style->black_gc, false,
			dragrect_origin_x, dragrect_origin_y,
			dragrect_current_x - dragrect_origin_x, dragrect_current_y - dragrect_origin_y);

	//draw_rect(widget, ctx, dragrect_origin_x, dragrect_current_x, dragrect_origin_y, dragrect_current_y);

	// turn on our new one
	gdk_draw_rectangle(ctx->render, widget->style->black_gc, false,
			dragrect_origin_x, dragrect_origin_y,
			x - dragrect_origin_x, y - dragrect_origin_y);
	dragrect_current_x = x;
	dragrect_current_y = y;

	gtk_widget_queue_draw (widget);
	// TODO queue_draw_area() instead

	//printf("button %d @ %d,%d\n", state, x, y);
	return TRUE;
}


/////////////////////////////////////////////////////////////////

static _gtk_ctx gtk_ctx;
static _render_ctx render_ctx;

int main (int argc, char**argv)
{
#define _ (char*)
	// TODO: replace with GtkUIManager.
	GtkItemFactoryEntry main_menu_items[] = {
			{ _"/_Main", 0, 0, 0, _"<Branch>" },
			{ _"/Main/_Params", _"<control>P", (GtkItemFactoryCallback)do_config, 0, _"<Item>" },
			{ _"/Main/_Quit", _"<control>Q", gtk_main_quit, 0, _"<Item>" },
	};
#undef _

	gint n_main_menu_items = sizeof (main_menu_items) / sizeof (main_menu_items[0]);

	GtkWidget *main_vbox, *menubar;
	GtkWidget *canvas;

	render_ctx.fractal = new Mandelbrot();
	render_ctx.centre = { -0.7, 0.0 };
	render_ctx.size = { 3.0, 3.0 };
	render_ctx.maxiter = 1000;
	// _main_ctx.pal initial setting by setup_colour_menu().

	gtk_ctx.mainctx = &render_ctx;
	g_thread_init(0);
	gdk_threads_init();
	gtk_init(&argc, &argv);
	GtkWidget * window = gtk_ctx.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), 0);
	g_signal_connect(window, "destroy", G_CALLBACK(destroy_event), 0);

	gtk_window_set_title(GTK_WINDOW(window), "brot2"); // will be updated by renderer..
	main_vbox = gtk_vbox_new(FALSE,1);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	menubar = make_menubar(gtk_ctx.window, main_menu_items, n_main_menu_items, &gtk_ctx);
	setup_colour_menu(&gtk_ctx, menubar, DiscretePalette::registry["Gradient RGB"]);

	canvas = gtk_drawing_area_new();
	gtk_widget_set_size_request (GTK_WIDGET(canvas), 300, 300);
	gtk_signal_connect (GTK_OBJECT (canvas), "expose_event",
			(GtkSignalFunc) expose_event, &gtk_ctx);
	gtk_signal_connect (GTK_OBJECT(canvas),"configure_event",
			(GtkSignalFunc) configure_event, &gtk_ctx);
	gtk_signal_connect (GTK_OBJECT (canvas), "motion_notify_event",
			(GtkSignalFunc) motion_notify_event, &gtk_ctx);
	gtk_signal_connect (GTK_OBJECT (canvas), "button_press_event",
			(GtkSignalFunc) button_press_event, &gtk_ctx);
	gtk_signal_connect (GTK_OBJECT (canvas), "button_release_event",
			(GtkSignalFunc) button_release_event, &gtk_ctx);

	gtk_widget_set_events (canvas, GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);

	gtk_ctx.statusbar = GTK_STATUSBAR(gtk_statusbar_new());
	gtk_statusbar_push(gtk_ctx.statusbar, 0, "Initialising");
	gtk_statusbar_set_has_resize_grip(gtk_ctx.statusbar, 0);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), canvas, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_vbox), GTK_WIDGET(gtk_ctx.statusbar), FALSE, FALSE, 0);

	gtk_widget_show_all(window);
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	delete render_ctx.plot;
	delete render_ctx.fractal;
	return 0;
}
