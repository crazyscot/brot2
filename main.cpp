/* main.cpp: main GTK program for brot2, license text follows */
static const char* license_text = "\
brot2: Yet Another Mandelbrot Plotter\n\
Copyright (C) 2010 Ross Younger\n\
\n\
This program is free software: you can redistribute it and/or modify \
it under the terms of the GNU General Public License as published by \
the Free Software Foundation, either version 3 of the License, or \
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License \
along with this program.  If not, see <http://www.gnu.org/licenses/>.";


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

static gint dragrect_origin_x, dragrect_origin_y,
			dragrect_active=0, dragrect_current_x, dragrect_current_y;

/*
 * TODO: Replace use of deprecated gtk UI calls:
 * replace gtk_signal_connect() with g_signal_connect()
 * convert to use cairo
 * use GtkUIManager instead of GtkItemFactory
 */

typedef struct _render_ctx {
	Plot * plot;
	DiscretePalette * pal;
	SmoothPalette * pals;
	// Yes, the following are mostly the same as in the Plot - but the plot is torn down and recreated frequently.
	Fractal *fractal;
	cdbl centre, size;
	unsigned width, height, maxiter;
	bool draw_hud;
	bool initializing; // Disables certain event actions when set.

	_render_ctx(): plot(0), pal(0), fractal(0), initializing(true) {};
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

static void draw_hud_gdk(GtkWidget * widget, _gtk_ctx *gctx)
{
	GdkPixmap *dest = gctx->render;
	_render_ctx * rctx = gctx->mainctx;
	PangoLayout * lyt = gtk_widget_create_pango_layout(widget,
			gctx->mainctx->plot->info_short().c_str());
	PangoFontDescription * fontdesc = pango_font_description_from_string ("Luxi Sans 9");
	pango_layout_set_font_description (lyt, fontdesc);
	pango_layout_set_width(lyt, PANGO_SCALE * rctx->width);
	pango_layout_set_wrap(lyt, PANGO_WRAP_WORD_CHAR);

	PangoRectangle rect;
	pango_layout_get_extents(lyt, &rect, 0);

	// N.B. If we want to use a gdkgc to actually render, need to use gdk_draw_text().
	int ploty = rctx->height - PANGO_PIXELS(rect.height) - PANGO_PIXELS(rect.y) - 1;

	GdkColor white = {0,65535,65535,65535}, black = {0,0,0,0};
	gdk_draw_layout_with_colors(dest, widget->style->white_gc,
			0, ploty,
			lyt, &white, &black);

	pango_font_description_free (fontdesc);
	g_object_unref (lyt);
}

static void render_gdk(GtkWidget * widget, _gtk_ctx *gctx) {
	GdkPixmap *dest = gctx->render;
	GdkGC *gc = widget->style->white_gc;
	_render_ctx * rctx = gctx->mainctx;

	const gint rowbytes = rctx->width * 3;
	const gint rowstride = rowbytes + 8-(rowbytes%8);
	guchar *buf = new guchar[rowstride * rctx->height]; // packed 24-bit data

	const fractal_point * data = rctx->plot->get_plot_data();
	assert(data);

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but gdk assumes a top-left origin.

	unsigned i;
	int j;
	for (j=rctx->height-1; j>=0; j--) {
		guchar *row = &buf[j*rowstride];
		for (i=0; i<rctx->width; i++) {
			if (data->iter == rctx->maxiter) {
				row[0] = row[1] = row[2] = 0;
			} else {
				rgb col;
				if (rctx->pal)
					col = (*rctx->pal)[data->iter];
				else if (rctx->pals)
					col = rctx->pals->get(*data);
				row[0] = col.r;
				row[1] = col.g;
				row[2] = col.b;
			}
			row += 3;
			++data;
		}
	}

	gdk_draw_rgb_image(dest, gc,
			0, 0, rctx->width, rctx->height, GDK_RGB_DITHER_NONE, buf, rowstride);

	if (rctx->draw_hud)
		draw_hud_gdk(widget, gctx);

	delete[] buf;
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


#define OPTIONS_DRAW_HUD "/Options/Draw HUD"

/* Factory-generates our main menubar widget.
 * To be converted to GtkUIManager... */
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

	/* Initial item state setup */
	GtkWidget *drawhud = gtk_item_factory_get_widget(item_factory, OPTIONS_DRAW_HUD);
	gtk_check_menu_item_set_active(
		GTK_CHECK_MENU_ITEM (drawhud),
		ctx->mainctx->draw_hud);
	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}


static void recolour(GtkWidget * widget, _gtk_ctx *ctx)
{
	render_gdk(widget, ctx);
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
	int i, clip=0, aspectfix=0;
	struct timeval tv_start, tv_after, tv_diff;
	double aspect;
	_gtk_ctx *ctx = (_gtk_ctx*)arg;

	aspect = (double)ctx->mainctx->width / ctx->mainctx->height;
	if (imag(ctx->mainctx->size) * aspect != real(ctx->mainctx->size)) {
		ctx->mainctx->size.imag(real(ctx->mainctx->size)/aspect);
		aspectfix=1;
	}
	if (fabs(real(ctx->mainctx->size)/ctx->mainctx->width) < MINIMUM_PIXEL_SIZE) {
		ctx->mainctx->size.real(MINIMUM_PIXEL_SIZE*ctx->mainctx->width);
		clip = 1;
	}
	if (fabs(imag(ctx->mainctx->size)/ctx->mainctx->height) < MINIMUM_PIXEL_SIZE) {
		ctx->mainctx->size.imag(MINIMUM_PIXEL_SIZE*ctx->mainctx->height);
		clip = 1;
	}

	// N.B. This (gtk/gdk lib calls from non-main thread) will not work at all on win32; will need to refactor if I ever port.
	gettimeofday(&tv_start,0);

	if (ctx->mainctx->plot) delete ctx->mainctx->plot;

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
			fprintf(stderr, "Could not start render thread %d: %d: %s\n", i, err->code, err->message);
			gdk_threads_enter();
			GtkWidget * dialog = gtk_message_dialog_new (GTK_WINDOW(ctx->window),
			                                 GTK_DIALOG_DESTROY_WITH_PARENT,
			                                 GTK_MESSAGE_ERROR,
			                                 GTK_BUTTONS_CLOSE,
			                                 "SEVERE: Could not start render thread %d: code %d: %s",
			                                 i, err->code, err->message);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			gdk_threads_leave();
			abort();
		}
	}

	for (i=0; i<NTHREADS; i++)
		g_thread_join(threads[i].thread); // ignore rv.

	// And now turn it into an RGB.
	gdk_threads_enter();
	recolour(ctx->window,ctx);
	gettimeofday(&tv_after,0);

	tv_diff = tv_subtract(tv_after, tv_start);
	double timetaken = tv_diff.tv_sec + (tv_diff.tv_usec / 1e6);

	gtk_statusbar_pop(ctx->statusbar, 0);
	std::ostringstream info;
	info << ctx->mainctx->plot->info_short();

	gtk_window_set_title(GTK_WINDOW(ctx->window), info.str().c_str());

	info.str("");
	info << "rendered in " << timetaken << "s.";
	if (aspectfix)
		info << " Aspect ratio autofixed.";
	if (clip)
		info << " Resolution limit reached, cannot zoom further!";

	gtk_statusbar_push(ctx->statusbar, 0, info.str().c_str());
	gtk_statusbar_pop(ctx->statusbar, 1);

	gtk_widget_queue_draw(ctx->window);
	gdk_flush();
	gdk_threads_leave();

	pthread_mutex_unlock(&ctx->render_lock);

	return 0;
}


// Redraws us onto a given widget (window), then queues an expose event
static void do_redraw_locked(GtkWidget *widget, _gtk_ctx *ctx)
{
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
		fprintf(stderr, "Could not start main render thread: %d: %s\n", err->code, err->message);
		gdk_threads_enter();
		GtkWidget * dialog = gtk_message_dialog_new (GTK_WINDOW(ctx->window),
		                                 GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 "SEVERE: Could not start main render thread: code %d: %s",
		                                 err->code, err->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		gdk_threads_leave();
		abort();
	}
}

static void do_redraw(GtkWidget *widget, _gtk_ctx *ctx)
{
	if (0 != pthread_mutex_trylock(&ctx->render_lock)) {
		gtk_statusbar_push(ctx->statusbar, 1, "Plot already in progress");
		return;
	}
	do_redraw_locked(widget,ctx);
}

static void do_resize(GtkWidget *widget, _gtk_ctx *ctx, unsigned width, unsigned height)
{
	if (0 != pthread_mutex_trylock(&ctx->render_lock)) {
		gtk_statusbar_push(ctx->statusbar, 1, "Plot already in progress");
		return;
	}

	if ((ctx->mainctx->width != width) ||
			(ctx->mainctx->height != height)) {
		// Size has changed!
		ctx->mainctx->width = width;
		ctx->mainctx->height = height;
		if (ctx->render) {
			g_object_unref(ctx->render);
			ctx->render = 0;
		}
	}
	do_redraw_locked(widget,ctx);
}


static void colour_menu_selection(_gtk_ctx *ctx, string lbl)
{
	DiscretePalette * sel = DiscretePalette::registry[lbl];
	if (sel != 0) {
		ctx->mainctx->pal = sel;
		ctx->mainctx->pals = 0;
	}
	SmoothPalette *sel2 = SmoothPalette::registry[lbl];
	if (sel2 != 0) {
		ctx->mainctx->pal = 0;
		ctx->mainctx->pals = sel2;
	}
	if (ctx->mainctx->plot)
		recolour(ctx->window, ctx);
}

static void colour_menu_selection1(gpointer *dat, GtkMenuItem *mi)
{
	const char * lbl = gtk_menu_item_get_label(mi);
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	colour_menu_selection(ctx, lbl);
}


static void setup_colour_menu(_gtk_ctx *ctx, GtkWidget *menubar, string initial)
{
	ctx->colour_menu = gtk_menu_new();

	GtkWidget *sep =  gtk_menu_item_new_with_label("Discrete");
	gtk_widget_set_sensitive(sep, false);
	gtk_menu_append(GTK_MENU(ctx->colour_menu), sep);

	std::map<std::string,DiscretePalette*>::iterator it;
	for (it = DiscretePalette::registry.begin(); it != DiscretePalette::registry.end(); it++) {
		GtkWidget * item = gtk_radio_menu_item_new_with_label(ctx->colours_radio_group, it->first.c_str());
		ctx->colours_radio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

		gtk_menu_append(GTK_MENU(ctx->colour_menu), item);
		if (0==strcmp(initial.c_str(),it->second->name.c_str())) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		}
		gtk_signal_connect_object(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(colour_menu_selection1), ctx);
		gtk_widget_show(item);
	}

	sep = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(ctx->colour_menu), sep);

	sep =  gtk_menu_item_new_with_label("Smooth");
	gtk_widget_set_sensitive(sep, false);
	gtk_menu_append(GTK_MENU(ctx->colour_menu), sep);

	std::map<std::string,SmoothPalette*>::iterator it2;
	for (it2 = SmoothPalette::registry.begin(); it2 != SmoothPalette::registry.end(); it2++) {
		GtkWidget * item = gtk_radio_menu_item_new_with_label(ctx->colours_radio_group, it2->first.c_str());
		ctx->colours_radio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

		gtk_menu_append(GTK_MENU(ctx->colour_menu), item);
		if (0==strcmp(initial.c_str(),it2->second->name.c_str())) {
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
	do_resize(widget, ctx, event->width, event->height);
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

static gboolean button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer *dat )
{
#define ZOOM_FACTOR 2.0f
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	if (ctx->render != NULL) {
		int redraw=0;
		cdbl new_ctr = ctx->mainctx->plot->pixel_to_set_tlo(event->x, event->y);

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
	int silly = 0;
	if (event->button != 8)
		return FALSE;

	//printf("button %d UP @ %d,%d\n", event->button, (int)event->x, (int)event->y);

	if (ctx->render != NULL) {
		int l = MIN(event->x, dragrect_origin_x);
		int r = MAX(event->x, dragrect_origin_x);
		int t = MIN(event->y, dragrect_origin_y);
		int b = MAX(event->y, dragrect_origin_y);

		if (abs(l-r)<2 || abs(t-b)<2) silly=1;

		// centres
		cdbl TR = ctx->mainctx->plot->pixel_to_set_tlo(r,t);
		cdbl BL = ctx->mainctx->plot->pixel_to_set_tlo(l,b);

		ctx->mainctx->centre = (TR+BL)/(long double)2.0;
		ctx->mainctx->size = TR - BL;

		dragrect_active = 0;

		if (!silly)
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

	//printf("button %d @ %d,%d\n", state, x, y);
	return TRUE;
}


/////////////////////////////////////////////////////////////////

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
	}
}

void do_config(gpointer _ctx, guint callback_action, GtkWidget *widget)
{
	_gtk_ctx * ctx = (_gtk_ctx*)_ctx;
	assert (ctx);
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

void toggle_hud(gpointer _ctx, guint callback_action, GtkWidget *widget)
{
	_gtk_ctx * ctx = (_gtk_ctx*)_ctx;
	assert (ctx);

	if (!ctx->mainctx->initializing) {
		// There must surely be a more idiomatic way to achieve the correct initial state??
		ctx->mainctx->draw_hud = !ctx->mainctx->draw_hud;
		do_redraw(ctx->window, ctx);
	}
}

/////////////////////////////////////////////////////////////////

void do_about(gpointer _ctx, guint callback_action)
{
	_gtk_ctx * ctx = (_gtk_ctx*)_ctx;
	gtk_show_about_dialog(GTK_WINDOW(ctx->window),
			"comments", "In memory of Benoît B. Mandelbrot.",
			"copyright", "(c) 2010 Ross Younger",
			"license", license_text,
			"wrap-license", TRUE, 
			"version", "0.01",
			NULL);
}

static _gtk_ctx gtk_ctx;
static _render_ctx render_ctx;

int main (int argc, char**argv)
{
#define _ (char*)
	GtkItemFactoryEntry main_menu_items[] = {
			{ _"/_Main", 0, 0, 0, _"<Branch>" },
			{ _"/_Main/_About", 0, (GtkItemFactoryCallback)do_about, 0, _"<Item>" },
			{ _"/Main/_Params", _"<control>P", (GtkItemFactoryCallback)do_config, 0, _"<Item>" },
			{ _"/Main/_Quit", _"<control>Q", gtk_main_quit, 0, _"<Item>" },
			{ _"/Options", 0, 0, 0, _"<Branch>" },
			{ _ OPTIONS_DRAW_HUD, 0, (GtkItemFactoryCallback)toggle_hud, 0, _"<CheckItem>" },
	};
#undef _

	gint n_main_menu_items = sizeof (main_menu_items) / sizeof (main_menu_items[0]);

	// Initial settings (set up BEFORE make_menubar):
	render_ctx.fractal = new Mandelbrot();
	render_ctx.centre = { -0.7, 0.0 };
	render_ctx.size = { 3.0, 3.0 };
	render_ctx.maxiter = 1000;

	render_ctx.draw_hud = true;
	// _main_ctx.pal initial setting by setup_colour_menu().

	gtk_ctx.mainctx = &render_ctx;

	GtkWidget *main_vbox, *menubar;
	GtkWidget *canvas;

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
	setup_colour_menu(&gtk_ctx, menubar, "Mid green");

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

	render_ctx.initializing = false;
	gtk_widget_show_all(window);
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	delete render_ctx.plot;
	delete render_ctx.fractal;
	return 0;
}
