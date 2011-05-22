/* main.cpp: main GTK program for brot2, license text follows */
static const char* license_text = "\
brot2: Yet Another Mandelbrot Plotter\n\
Copyright (c) 2010-2011 Ross Younger\n\
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

static const char *copyright_string = "(c) 2010-2011 Ross Younger";


#define _GNU_SOURCE 1
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <png.h>

#include <sys/time.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <complex>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>

#include "libbrot2.h"
#include "Plot2.h"
#include "palette.h"
#include "version.h"
#include "misc.h"
#include "logo.h"
#include "uixml.h"

#include <X11/Xlib.h>

using namespace std;

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

static gint dragrect_origin_x, dragrect_origin_y,
			dragrect_active=0, dragrect_current_x, dragrect_current_y;

#define DEFAULT_ANTIALIAS_FACTOR 2

class pixpack_format {
	// A pixel format identifier that supersets Cairo's.
	int f; // Pixel format - one of cairo_format_t or our internal constants
	public:
		static const int PACKED_RGB_24 = CAIRO_FORMAT_RGB16_565 + 1000000;
		pixpack_format(int c): f(c) {};
		inline operator int() const { return f; }
};

typedef struct _render_ctx {
	Plot2 * plot;
	Plot2 * plot_prev;
	BasePalette * pal;
	// Yes, the following are mostly the same as in the Plot - but the plot is torn down and recreated frequently.
	Fractal *fractal;
	cfpt centre, size;
	unsigned rwidth, rheight; // Rendering dimensions; plot dims will be larger if antialiased
	bool draw_hud, antialias;
	unsigned antialias_factor;
	bool initializing; // Disables certain event actions when set.

	_render_ctx(): antialias_factor(DEFAULT_ANTIALIAS_FACTOR), initializing(true) {};
	~_render_ctx() { if (plot) delete plot; if (plot_prev) delete plot_prev; }

} _render_ctx;

typedef struct _gtk_ctx : Plot2::callback_t {
	_render_ctx * mainctx;
	GtkWidget *window;
	cairo_surface_t *canvas, *hud, *dragrect;
	GtkProgressBar* progressbar;
	GtkWidget *colour_menu, *fractal_menu;
	GSList *colours_radio_group, *fractal_radio_group;
	guchar *imgbuf;

	struct timeval tv_start; // When did the current render start?
	bool aspectfix, clip; // Details about the current render

	_gtk_ctx() : mainctx(0), window(0), canvas(0), hud(0), dragrect(0),
		colour_menu(0), colours_radio_group(0), fractal_radio_group(0),
	    imgbuf(0)	{};

	virtual void plot_progress_minor(Plot2& plot, float workdone);
	virtual void plot_progress_major(Plot2& plot, unsigned current_max, string& commentary);
	virtual void plot_progress_complete(Plot2& plot);
} _gtk_ctx;

#define ALERT_DIALOG(parent, msg, args...) do { \
	GtkWidget * dialog = gtk_message_dialog_new (GTK_WINDOW(ctx->window), 	\
	                                 GTK_DIALOG_DESTROY_WITH_PARENT, 		\
	                                 GTK_MESSAGE_ERROR,						\
	                                 GTK_BUTTONS_CLOSE,						\
	                                 msg, ## args);							\
	gtk_dialog_run (GTK_DIALOG (dialog));									\
	gtk_widget_destroy (dialog);											\
} while(0)

#define RGB_BYTES_PER_PIXEL 3

static void clear_hud(cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_source_rgba(cairo, 0,0,0,0);
	cairo_set_operator (cairo, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

static void draw_hud_cairo(_gtk_ctx *gctx)
{
	_render_ctx * rctx = gctx->mainctx;
	if (!gctx->mainctx->plot) return; // race condition trap
	string info = gctx->mainctx->plot->info(true);

	if (!gctx->hud)
		gctx->hud = gdk_window_create_similar_surface(gctx->window->window,
				CAIRO_CONTENT_COLOR_ALPHA,
				gctx->mainctx->rwidth, gctx->mainctx->rheight);
	cairo_t *cairo = cairo_create(gctx->hud);
	clear_hud(cairo);
	PangoLayout * lyt = pango_cairo_create_layout(cairo);

	PangoFontDescription * fontdesc = pango_font_description_from_string ("Luxi Sans 9");
	pango_layout_set_font_description (lyt, fontdesc);
	pango_layout_set_text(lyt, info.c_str(), -1);
	pango_layout_set_width(lyt, PANGO_SCALE * rctx->rwidth);
	pango_layout_set_wrap(lyt, PANGO_WRAP_WORD_CHAR);

	PangoRectangle rect;
	pango_layout_get_extents(lyt, &rect, 0);

	PangoLayoutIter *iter = pango_layout_get_iter(lyt);
	do {
		PangoRectangle log;
		pango_layout_iter_get_line_extents(iter, 0, &log);
		cairo_save(cairo);
		cairo_set_source_rgb(cairo, 0,0,0);
		// NB. there are PANGO_SCALE pango units to the device unit.
		cairo_rectangle(cairo, log.x / PANGO_SCALE, log.y / PANGO_SCALE,
				log.width / PANGO_SCALE, log.height / PANGO_SCALE);
		cairo_clip(cairo);
		cairo_paint(cairo);
		cairo_restore(cairo);
	} while (pango_layout_iter_next_line(iter));

	cairo_move_to(cairo, 0.0, 0.0);
	cairo_set_source_rgb(cairo, 1.0,1.0,1.0);
	pango_cairo_show_layout (cairo, lyt);

	pango_font_description_free (fontdesc);
	pango_layout_iter_free(iter);
	g_object_unref (lyt);
	cairo_destroy(cairo);
}

/*
 * Renders a single pixel, given the current idea of infinity and the palette to use.
 */
static inline rgb render_pixel(const fractal_point *data, const int local_inf, const BasePalette * pal) {
	if (data->iter == local_inf || data->iter<0) {
		return black;
	} else {
		return pal->get(*data);
	}
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
static bool render_plot_generic(guchar *buf, const _render_ctx *rctx, const gint rowstride, const int local_inf, pixpack_format fmt)
{
	assert(buf);
	assert((unsigned)rowstride >= RGB_BYTES_PER_PIXEL * rctx->rwidth);

	const fractal_point * data = rctx->plot->get_data();
	if (!rctx->plot || !data) return false; // Oops, disappeared under our feet

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but gdk assumes a top-left origin.

	const unsigned factor = rctx->antialias ? rctx->antialias_factor : 1;

	const fractal_point ** srcs = new const fractal_point* [ factor ];
	unsigned i,j;
	for (j=0; j<rctx->rheight; j++) {
		guchar *dst = &buf[j*rowstride];
		const unsigned src_idx = (rctx->rheight - j - 1) * factor;
		for (unsigned k=0; k < factor; k++) {
			srcs[k] = &data[rctx->plot->width * (src_idx+k)];
		}

		for (i=0; i<rctx->rwidth; i++) {
			unsigned rr=0, gg=0, bb=0; // Accumulate the result

			for (unsigned k=0; k < factor; k ++) {
				for (unsigned l=0; l < factor; l++) {
					rgb pix1 = render_pixel(&srcs[k][l], local_inf, rctx->pal);
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

static void plot_to_png(_gtk_ctx *ctx, char *filename)
{
	FILE *f;

	f = fopen(filename, "wb");
	if (f==NULL) {
		ALERT_DIALOG(ctx->window, "Could not open file for writing: %d (%s)", errno, strerror(errno));
		return;
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop png_info=0;
	if (png)
		png_info = png_create_info_struct(png);
	if (!png_info) {
		ALERT_DIALOG(ctx->window, "Could not create PNG structs (out of memory?)");
		if (png)
			png_destroy_write_struct(&png, 0);
		return;
	}
	jmp_buf jbuf;
	if (setjmp(jbuf)) {
		fclose(f);
		return;
	}

	const unsigned width = ctx->mainctx->rwidth,
			height = ctx->mainctx->rheight;

	png_init_io(png, f);

	png_set_compression_level(png, Z_BEST_SPEED);
	png_set_IHDR(png, png_info,
			width, height, 8 /* 24bpp */,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	string comment = ctx->mainctx->plot->info(true);
	const char* SOFTWARE = "brot2",
		      * INFO = comment.c_str();
	png_text texts[2] = {
			{PNG_TEXT_COMPRESSION_NONE, (char*)"Software", (char*)SOFTWARE, strlen(SOFTWARE)},
			{PNG_TEXT_COMPRESSION_NONE, (char*)"Comment", (char*)INFO, strlen(INFO) },
	};
	png_set_text(png, png_info, texts, 2);

	png_write_info(png, png_info);

	const int rowstride = RGB_BYTES_PER_PIXEL * width;

	guchar * pngbuf = new guchar[rowstride * height];
	render_plot_generic(pngbuf, ctx->mainctx, rowstride, -1, pixpack_format(pixpack_format::PACKED_RGB_24));
	guchar ** pngrows = new guchar*[height];
	for (unsigned i=0; i<height; i++) {
		pngrows[i] = pngbuf + rowstride*i;
	}
	png_write_image(png, pngrows);
	delete[] pngrows;
	delete[] pngbuf;
	png_write_end(png,png_info);
	png_destroy_write_struct(&png, &png_info);

	if (0==fclose(f))
		gtk_progress_bar_set_text(ctx->progressbar, "Successfully saved");
	else
		ALERT_DIALOG(ctx->window, "Error closing the save: %d: %s", errno, strerror(errno));
}

static string last_saved_dirname;

static void do_save(GtkAction *UNUSED(action), _gtk_ctx* ctx)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new ("Save File",
					      GTK_WINDOW(ctx->window),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

	if (!last_saved_dirname.length()) {
		ostringstream str;
		str << "/home/" << getenv("USERNAME") << "/Documents/";
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), str.str().c_str());
	} else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), last_saved_dirname.c_str());
	}

	{
		ostringstream str;
		str << ctx->mainctx->plot->info().c_str() << ".png";
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), str.str().c_str());
	}

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    	char *filename;
    	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    	last_saved_dirname.clear();
    	plot_to_png(ctx, filename);

    	last_saved_dirname.append(filename);
    	int spos = last_saved_dirname.rfind('/');
    	if (spos >= 0)
    		last_saved_dirname.erase(spos+1);

    	g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

static void render_cairo(GtkWidget * UNUSED(widget), _gtk_ctx *gctx, int local_inf=-1) {
	_render_ctx * rctx = gctx->mainctx;

	const cairo_format_t FORMAT = CAIRO_FORMAT_RGB24;
	const gint rowstride = cairo_format_stride_for_width(FORMAT, gctx->mainctx->rwidth);


	if (!gctx->imgbuf)
		gctx->imgbuf = new guchar[rowstride * rctx->rheight];

	// TODO: autolock on gctx ? and everything that accessess gctx->(surfaces)?
	cairo_surface_t *surface;
	if (gctx->canvas) {
		surface = gctx->canvas;
		cairo_surface_reference(surface);
	} else {
		surface = cairo_image_surface_create_for_data(gctx->imgbuf, FORMAT,
				gctx->mainctx->rwidth, gctx->mainctx->rheight, rowstride);
		{
			cairo_status_t st = cairo_surface_status(surface);
			if (st != 0) {
				cerr << "Surface error: " << cairo_status_to_string(st) << endl;
			}
		}

		cairo_surface_reference(surface);
		gctx->canvas = surface;
	}

	guchar *t = cairo_image_surface_get_data(surface);
	if (!t)
		cerr << "Surface data is NULL!" <<endl;
	{
		cairo_status_t st = cairo_surface_status(surface);
		if (st != 0) {
			cerr << "Surface error: " << cairo_status_to_string(st) << endl;
		}
	}

	if (!render_plot_generic(cairo_image_surface_get_data(surface),
				rctx, rowstride, local_inf,
				FORMAT)) { // Oops, it vanished
		goto done;
	}
	cairo_surface_mark_dirty(surface);

	if (rctx->draw_hud)
		draw_hud_cairo(gctx);
	else {
		if (gctx->hud) {
			cairo_surface_destroy(gctx->hud);
			gctx->hud = 0;
		}
	}

done:
	cairo_surface_destroy(surface); // But we referenced it after creation.
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

static gboolean delete_event(GtkWidget *UNUSED(widget), GdkEvent *UNUSED(event), gpointer UNUSED(data))
{
	return FALSE;
}

static void destroy_event(GtkWidget *UNUSED(widget), gpointer UNUSED(data))
{
	gtk_main_quit();
}

static void recolour(GtkWidget * widget, _gtk_ctx *ctx)
{
	render_cairo(widget, ctx);
	gtk_widget_queue_draw(widget);
}

void _gtk_ctx::plot_progress_minor(Plot2& UNUSED(plot), float workdone) {
	gdk_threads_enter();
	gtk_progress_bar_set_fraction(progressbar, workdone);
	gdk_threads_leave();
}

void _gtk_ctx::plot_progress_major(Plot2& UNUSED(plot), unsigned current_max, string& commentary) {
	render_cairo(window, this, current_max);
	gdk_threads_enter();
	gtk_progress_bar_set_fraction(progressbar, 0.98);
	gtk_progress_bar_set_text(progressbar, commentary.c_str());
	gtk_widget_queue_draw(window);
	gdk_threads_leave();
}

void _gtk_ctx::plot_progress_complete(Plot2& plot) {
	struct timeval tv_after, tv_diff;

	gdk_threads_enter();
	gtk_progress_bar_pulse(progressbar);
	recolour(window,this);
	gettimeofday(&tv_after,0);

	tv_diff = tv_subtract(tv_after, tv_start);
	double timetaken = tv_diff.tv_sec + (tv_diff.tv_usec / 1e6);

	gtk_window_set_title(GTK_WINDOW(window), plot.info(false).c_str());

	std::ostringstream info;
	info.precision(4);
	info << timetaken << "s; ";
	info << plot.get_passes() <<" passes; maxiter=" << plot.get_maxiter() << ".";
	if (aspectfix)
		info << " Aspect ratio autofixed.";
	if (clip)
		info << " Resolution limit reached, cannot zoom further!";
	clip = aspectfix = false;

	gtk_progress_bar_set_fraction(progressbar, 1.0);
	gtk_progress_bar_set_text(progressbar, info.str().c_str());

	gtk_widget_queue_draw(window);
	gdk_flush();
	gdk_threads_leave();
}

static void safe_stop_plot(Plot2 * p) {
	// As it stands this function must only ever be called from the main thread.
	// If this assumption later fails to hold, need to vary it to not
	// twiddle the gdk_threads lock.
	if (p) {
		gdk_threads_leave();
		p->stop();
		p->wait();
		gdk_threads_enter();
	}
}

// (Re)draws us onto a given widget (window), then sets up to queue an expose event when it's done.
static void do_plot(GtkWidget *widget, _gtk_ctx *ctx, bool is_same_plot = false)
{
	safe_stop_plot(ctx->mainctx->plot);

	if (!ctx->canvas) {
		// First time? Grey it all out.
		cairo_t *cairo = gdk_cairo_create(widget->window);
		cairo_set_source_rgb(cairo, 0.9, 0.1, 0.1);
		cairo_paint(cairo);
		cairo_destroy(cairo);
	}

	gtk_progress_bar_set_text(ctx->progressbar, "Plotting pass 1...");

	double aspect;

	aspect = (double)ctx->mainctx->rwidth / ctx->mainctx->rheight;
	if (imag(ctx->mainctx->size) * aspect != real(ctx->mainctx->size)) {
		ctx->mainctx->size.imag(real(ctx->mainctx->size)/aspect);
		ctx->aspectfix=1;
	}
	if (fabs(real(ctx->mainctx->size)/ctx->mainctx->rwidth) < MINIMUM_PIXEL_SIZE) {
		ctx->mainctx->size.real(MINIMUM_PIXEL_SIZE*ctx->mainctx->rwidth);
		ctx->clip = 1;
	}
	if (fabs(imag(ctx->mainctx->size)/ctx->mainctx->rheight) < MINIMUM_PIXEL_SIZE) {
		ctx->mainctx->size.imag(MINIMUM_PIXEL_SIZE*ctx->mainctx->rheight);
		ctx->clip = 1;
	}

	// N.B. This (gtk/gdk lib calls from non-main thread) will not work at all on win32; will need to refactor if I ever port.
	gettimeofday(&ctx->tv_start,0);

	Plot2 * deleteme = 0;
	if (is_same_plot) {
		deleteme = ctx->mainctx->plot;
		ctx->mainctx->plot = 0;
	} else {
		if (ctx->mainctx->plot_prev)
			deleteme = ctx->mainctx->plot_prev;
		ctx->mainctx->plot_prev = ctx->mainctx->plot;
		ctx->mainctx->plot = 0;
	}

	if (deleteme) {
		// Must release the mutex as the thread join may block...
		gdk_threads_leave();
		delete deleteme;
		gdk_threads_enter();
	}

	assert(!ctx->mainctx->plot);
	unsigned pwidth = ctx->mainctx->rwidth,
			pheight = ctx->mainctx->rheight;
	if (ctx->mainctx->antialias) {
		pwidth *= ctx->mainctx->antialias_factor;
		pheight *= ctx->mainctx->antialias_factor;
	}
	ctx->mainctx->plot = new Plot2(ctx->mainctx->fractal, ctx->mainctx->centre, ctx->mainctx->size, pwidth, pheight);
	ctx->mainctx->plot->start(ctx);
	// TODO try/catch (and in do_resume) - report failure. Is gtkmm exception-safe?
}

static void do_resume(GtkWidget *UNUSED(widget), _gtk_ctx *ctx)
{
	assert(ctx->canvas);
	assert(ctx->mainctx->plot);
	// If either of these assertions fail, we're in a tight corner case
	// and it might be better to just ignore the resume request?

	if (!ctx->mainctx->plot->is_done()) {
		gtk_progress_bar_set_text(ctx->progressbar, "Plot already running");
		return;
	}

	gettimeofday(&ctx->tv_start,0);
	ctx->mainctx->plot->start(ctx, true);
}

static void do_resize(GtkWidget *widget, _gtk_ctx *ctx, unsigned width, unsigned height)
{
	safe_stop_plot(ctx->mainctx->plot);

	if ((ctx->mainctx->rwidth != width) ||
			(ctx->mainctx->rheight != height)) {
		// Size has changed!
		ctx->mainctx->rwidth = width;
		ctx->mainctx->rheight = height;
		if (ctx->canvas) {
			cairo_surface_destroy(ctx->canvas);
			ctx->canvas = 0;
		}
		if (ctx->imgbuf) {
			delete[] ctx->imgbuf;
			ctx->imgbuf=0;
		}
		if (ctx->hud) {
			cairo_surface_destroy(ctx->hud);
			ctx->hud = 0;
		}
		if (ctx->dragrect) {
			cairo_surface_destroy(ctx->dragrect);
			ctx->dragrect = 0;
		}
	}
	do_plot(widget,ctx);
}

static void colour_menu_selection(_gtk_ctx *ctx, string lbl)
{
	DiscretePalette * selS = DiscretePalette::registry[lbl];
	SmoothPalette *selD = SmoothPalette::registry[lbl];
	if (selS) {
		ctx->mainctx->pal = selS;
	} else if (selD) {
		ctx->mainctx->pal = selD;
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
		g_signal_connect_swapped(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(colour_menu_selection1), ctx);
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
		g_signal_connect_swapped(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(colour_menu_selection1), ctx);
		gtk_widget_show(item);
	}

	colour_menu_selection(ctx, initial);

    GtkWidget* colour_item = gtk_menu_item_new_with_mnemonic("_Colour");
    gtk_widget_show (colour_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), colour_item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (colour_item), ctx->colour_menu);
}

static void fractal_menu_selection(_gtk_ctx *ctx, string lbl)
{
	Fractal *sel = Fractal::registry()[lbl];
	if (sel) {
		ctx->mainctx->fractal = sel;
		if (ctx->mainctx->plot)
			do_plot(ctx->window, ctx);
	}
}

static void fractal_menu_selection1(gpointer *dat, GtkMenuItem *mi)
{
	const char * lbl = gtk_menu_item_get_label(mi);
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	fractal_menu_selection(ctx, lbl);
}

static void setup_fractal_menu(_gtk_ctx *ctx, GtkWidget *menubar, string initial)
{
	ctx->fractal_menu = gtk_menu_new();

	std::map<std::string,Fractal*>::iterator it;
	unsigned maxsortorder = 0;
	for (it = Fractal::registry().begin(); it != Fractal::registry().end(); it++) {
		if (it->second->sortorder > maxsortorder)
			maxsortorder = it->second->sortorder;
	}

	unsigned sortpass=0;
	for (sortpass=0; sortpass <= maxsortorder; sortpass++) {
		for (it = Fractal::registry().begin(); it != Fractal::registry().end(); it++) {
			if (it->second->sortorder != sortpass)
				continue;

			GtkWidget * item = gtk_radio_menu_item_new_with_label(ctx->fractal_radio_group, it->first.c_str());
			ctx->fractal_radio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

			gtk_menu_append(GTK_MENU(ctx->fractal_menu), item);
			if (0==strcmp(initial.c_str(),it->second->name.c_str())) {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
			}
			g_signal_connect_swapped(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(fractal_menu_selection1), ctx);
			gtk_widget_set_tooltip_text(item, it->second->description.c_str());
			gtk_widget_show(item);
		}
	}

	fractal_menu_selection(ctx, initial);

    GtkWidget* fractal_item = gtk_menu_item_new_with_mnemonic ("_Fractal");
    gtk_widget_show (fractal_item);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (fractal_item), ctx->fractal_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fractal_item);
}


static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer *dat)
{
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	do_resize(widget, ctx, event->width, event->height);
	return TRUE;
}

static gboolean expose_event( GtkWidget *widget, GdkEventExpose *UNUSED(event), gpointer *dat )
{
	cairo_t *dest = gdk_cairo_create(widget->window);
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	if (!ctx->canvas) return TRUE; // On startup? Nothing we can do
	assert(ctx->canvas); // If this fails then we need to handle it better. Check to see if we're being called concurrently with a plot request; if so then lock or ignore.

	/* We might wish to redraw only the exposed area, for efficiency.
	 * However there's an efficiency trap in some circumstances, so let's
	 * only do this if we really think we need to.
	cairo_rectangle(dest, event->area.x, event->area.y,
	                    event->area.width, event->area.height);
	cairo_clip(dest);
	*/

	cairo_set_source_surface(dest, ctx->canvas, 0, 0);
	cairo_paint(dest);

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

	cairo_destroy(dest);
	return FALSE;
}

static void do_undo(GtkAction *UNUSED(action), _gtk_ctx *ctx)
{
	if (ctx->canvas == NULL) return;
	if (!ctx->mainctx->plot_prev) {
		gtk_progress_bar_set_text(ctx->progressbar, "Nothing to undo");
		return;
	}

	_render_ctx * main = ctx->mainctx;
	Plot2 * tmp = main->plot;
	main->plot = main->plot_prev;
	main->plot_prev = tmp;

	main->centre = main->plot->centre;
	main->size = main->plot->size;
	main->rwidth = main->plot->width;
	main->rheight = main->plot->height;
	recolour(ctx->window, ctx);
}

enum zooms {
	REDRAW_ONLY=1,
	ZOOM_IN,
	ZOOM_OUT,
};

#define ZOOM_FACTOR 2.0f
static void do_zoom(_gtk_ctx *ctx, enum zooms type)
{
	if (ctx->canvas != NULL) {
		switch (type) {
			case ZOOM_IN:
				ctx->mainctx->size /= ZOOM_FACTOR;
				break;
			case ZOOM_OUT:
				ctx->mainctx->size *= ZOOM_FACTOR;
				{
					fvalue d = real(ctx->mainctx->size), lim = ctx->mainctx->fractal->xmax - ctx->mainctx->fractal->xmin;
					if (d > lim)
						ctx->mainctx->size.real(lim);
					d = imag(ctx->mainctx->size), lim = ctx->mainctx->fractal->ymax - ctx->mainctx->fractal->ymin;
					if (d > lim)
						ctx->mainctx->size.imag(lim);
				}
				break;
			case REDRAW_ONLY:
				break;
		}
		do_plot(ctx->window, ctx);
	}
}

static void do_zoom_in(GtkAction *UNUSED(a), _gtk_ctx *ctx)
{
	do_zoom(ctx, ZOOM_IN);
}

static void do_zoom_out(GtkAction *UNUSED(a), _gtk_ctx *ctx)
{
	do_zoom(ctx, ZOOM_OUT);
}

cfpt pixel_to_set_tlo(_render_ctx *ctx, int x, int y)
{
	if (ctx->antialias) {
		// scale up our click to the plot point within
		x *= ctx->antialias_factor;
		y *= ctx->antialias_factor;
	}
	return ctx->plot->pixel_to_set_tlo(x,y);
}

static gboolean button_press_event( GtkWidget *UNUSED(widget), GdkEventButton *event, gpointer *dat )
{
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	if (ctx->canvas != NULL) {
		cfpt new_ctr = pixel_to_set_tlo(ctx->mainctx, event->x, event->y);

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

	return FALSE;
}

// Map keypad + and - into their accelerator counterparts.
static gboolean key_press_event( GtkWidget *UNUSED(widget), GdkEventKey *event, gpointer *dat )
{
	_gtk_ctx * ctx = (_gtk_ctx*) dat;
	switch(event->keyval) {
		case GDK_KP_Add:
			do_zoom(ctx, ZOOM_IN);
			return TRUE;
		case GDK_KP_Subtract:
			do_zoom(ctx, ZOOM_OUT);
			return TRUE;
	}
	return FALSE;
}

static gboolean button_release_event( GtkWidget *UNUSED(widget), GdkEventButton *event, gpointer *dat )
{
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
			cfpt TR = pixel_to_set_tlo(ctx->mainctx, r, t);
			cfpt BL = pixel_to_set_tlo(ctx->mainctx, l, b);
			ctx->mainctx->centre = (TR+BL)/(fvalue)2.0;
			ctx->mainctx->size = TR - BL;

			do_plot(ctx->window, ctx);
		}
	}
	return TRUE;
}

static void clear_dragrect(cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_source_rgba(cairo, 0,0,0,0);
	cairo_set_operator (cairo, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

// Draw my rectangle. Negative dimensions are handled properly.
static void draw_dragrect(cairo_t *cairo,
		gint x, gint y, gint width, gint height)
{
	if (width < 0) {
		x += width;
		width = -width;
	}
	if (height < 0) {
		y += height;
		height = -height;
	}
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
	cairo_rectangle(cairo, x, y, width, height);
	cairo_set_line_width(cairo, 1.5);

	const double dashes[] = { 5.0, 5.0 };

	// First do it in black...
	cairo_set_source_rgb(cairo, 0,0,0);
	cairo_set_dash(cairo, dashes, sizeof(dashes), 0.0);
	cairo_stroke(cairo);

	// ... then overlay with white dashes. The call back to cairo_rectangle
	// seems necessary for the overlay to not throw away what we just did.
	cairo_rectangle(cairo, x, y, width, height);
	cairo_set_source_rgb(cairo, 1,1,1);
	cairo_set_dash(cairo, dashes, sizeof(dashes), 5.0);
	cairo_stroke(cairo);

	cairo_restore(cairo);
}

static gboolean motion_notify_event( GtkWidget *widget, GdkEventMotion *event, gpointer * _dat)
{
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
}


/////////////////////////////////////////////////////////////////

static void update_entry_float(GtkWidget *entry, fvalue val)
{
	ostringstream tmp;
	tmp.precision(MAXIMAL_DECIMAL_PRECISION);
	tmp << val;
	gtk_entry_set_text(GTK_ENTRY(entry), tmp.str().c_str());
}

static bool read_entry_float(GtkWidget *entry, fvalue *val_out)
{
	const gchar * raw = gtk_entry_get_text(GTK_ENTRY(entry));
	istringstream tmp(raw, istringstream::in);
	tmp.precision(MAXIMAL_DECIMAL_PRECISION);
	fvalue rv=0;

	tmp >> rv;
	if (tmp.fail())
		return false;

	*val_out = rv;
	return true;
}

void do_params_dialog(GtkAction *UNUSED(action), _gtk_ctx *ctx)
{
	assert (ctx);

	GtkWidget *dlg = gtk_dialog_new_with_buttons("Plot parameters", GTK_WINDOW(ctx->window),
			GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			NULL);
	GtkWidget * content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	GtkWidget * label;

	//label = gtk_label_new ("Configuration");
	//gtk_container_add (GTK_CONTAINER (content_area), label);

	GtkWidget *c_re, *c_im, *size_re;
	c_re = gtk_entry_new();
	update_entry_float(c_re, real(ctx->mainctx->centre));
	c_im = gtk_entry_new();
	update_entry_float(c_im, imag(ctx->mainctx->centre));
	size_re = gtk_entry_new();
	update_entry_float(size_re, real(ctx->mainctx->size));

	GtkWidget * box = gtk_table_new(3, 2, FALSE);

	label = gtk_label_new("Centre Real (x) ");
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(box), c_re, 1, 2, 0, 1);

	label = gtk_label_new("Centre Imaginary (y) ");
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(box), c_im, 1, 2, 1, 2);

	label = gtk_label_new("Real (x) axis length ");
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(box), size_re, 1, 2, 2, 3);
	// Don't bother with imaginary axis length, it's implicit from the aspect ratio.

	gtk_container_add (GTK_CONTAINER (content_area), box);

	gtk_widget_show_all(dlg);
	bool error;
	gint result;
	do {
		error = false;
		result = gtk_dialog_run(GTK_DIALOG(dlg));
		if (result == GTK_RESPONSE_ACCEPT) {
			fvalue res=0;
			if (read_entry_float(c_re, &res))
				ctx->mainctx->centre.real(res);
			else error = true;
			if (read_entry_float(c_im, &res))
				ctx->mainctx->centre.imag(res);
			else error = true;
			if (read_entry_float(size_re, &res))
				ctx->mainctx->size.real(res);
			else error = true;
			// imaginary axis length is implicit.

			if (error) {
				GtkWidget * errdlg = gtk_message_dialog_new (GTK_WINDOW(dlg),
				                                 GTK_DIALOG_DESTROY_WITH_PARENT,
				                                 GTK_MESSAGE_QUESTION,
				                                 GTK_BUTTONS_OK,
				                                 "Sorry, I could not parse that; care to try again?");
				gtk_dialog_run (GTK_DIALOG (errdlg));
				gtk_widget_destroy (errdlg);
			} else
				do_plot(ctx->window, ctx);
		}
	} while (error && result == GTK_RESPONSE_ACCEPT);
	gtk_widget_destroy(dlg);
}

static void toggle_hud(GtkAction *UNUSED(Action), _gtk_ctx *ctx)
{
	if (ctx->mainctx->initializing) return;
	ctx->mainctx->draw_hud = !ctx->mainctx->draw_hud;
	recolour(ctx->window, ctx);
}

static void toggle_antialias(GtkAction *UNUSED(Action), _gtk_ctx *ctx)
{
	if (ctx->mainctx->initializing) return;
	safe_stop_plot(ctx->mainctx->plot);
	ctx->mainctx->antialias = !ctx->mainctx->antialias;
	do_plot(ctx->window, ctx, false);
}

void do_stop_plot(GtkAction *UNUSED(Action), _gtk_ctx *ctx)
{
	assert (ctx);
	gtk_progress_bar_set_text(ctx->progressbar, "Stopping...");
	safe_stop_plot(ctx->mainctx->plot);
	gtk_progress_bar_set_text(ctx->progressbar, "Stopped at user request");
	gtk_progress_bar_set_fraction(ctx->progressbar,0);
}

void do_refresh_plot(GtkAction *UNUSED(Action), _gtk_ctx *ctx)
{
	assert (ctx);
	do_plot(ctx->window, ctx, true);
}

void do_plot_more(GtkAction *UNUSED(Action), _gtk_ctx *ctx)
{
	assert (ctx);
	do_resume(ctx->window, ctx);
}

/////////////////////////////////////////////////////////////////

void do_about(GtkAction *UNUSED(action), _gtk_ctx *ctx)
{
	GdkPixbuf *logo = gdk_pixbuf_new_from_inline(-1, brot2_logo, FALSE, 0);
	gtk_show_about_dialog(GTK_WINDOW(ctx->window),
			"comments", "Dedicated to the memory of Benoît B. Mandelbrot.",
			"copyright", copyright_string,
			"license", license_text,
			"wrap-license", TRUE, 
			"version", BROT2_VERSION_STRING,
			"logo", logo,
			NULL);
	g_object_unref(logo); // gdk_pixmap_unref
}

static _gtk_ctx gtk_ctx;
static _render_ctx render_ctx;

static struct option longopts[] = {
	{"version", 0, 0, 'v'},
	{ 0,0,0,0 }
};

int main (int argc, char**argv)
{
	XInitThreads();

	int opt, optind = 0;
	while ((opt = getopt_long(argc, argv, "v", longopts, &optind)) != -1) {
		switch(opt) {
			case 'v':
			case '?':
				cout << "brot2 v" << BROT2_VERSION_STRING << endl;
				cout << "Copyright " << copyright_string << endl;
				return 0;
		}
	}
	if (argc > optind+1) {
		cerr << "This program does not take any arguments." << endl;
		return 1;
	}

	g_thread_init(0);
	gdk_threads_init();
	gdk_threads_enter();
	gtk_init(&argc, &argv);

	// Initial settings (set up BEFORE the menubar):
	render_ctx.centre = { 0.0, 0.0 };
	render_ctx.size = { 4.5, 4.5 };

	render_ctx.draw_hud = true;
	render_ctx.antialias = false;
	// _main_ctx.pal initial setting by setup_colour_menu().
	// render_ctx.fractal set by setup_fractal_menu().

	static GtkActionEntry entries[] = {
		{ "MainMenuAction", NULL, "_Main", 0,0,0 },
		{ "AboutAction", GTK_STOCK_ABOUT, "_About", 0, 0, G_CALLBACK(do_about) },
		{ "SaveImageAction", GTK_STOCK_SAVE, "_Save image...", "<control>S", 0, G_CALLBACK(do_save) },
		{ "QuitAction", GTK_STOCK_QUIT, "_Quit", "<control>Q", 0, gtk_main_quit} ,

		{ "OptionsMenuAction", 0, "_Options", 0,0,0 },

		{ "PlotMenuAction", 0, "_Plot", 0,0,0 },
		{ "UndoAction", GTK_STOCK_UNDO, "_Undo", "<control>Z", 0, G_CALLBACK(do_undo) },
		{ "ParametersAction", GTK_STOCK_PROPERTIES, "_Parameters", "<control>P", 0, G_CALLBACK(do_params_dialog) },
		{ "ZoomInAction", GTK_STOCK_ZOOM_IN, "Zoom _In", "plus", 0, G_CALLBACK(do_zoom_in) },
		{ "ZoomOutAction", GTK_STOCK_ZOOM_OUT, "Zoom _Out", "minus", 0, G_CALLBACK(do_zoom_out) },
		{ "StopAction", GTK_STOCK_CANCEL, "Stop", "<control>period", 0, G_CALLBACK(do_stop_plot) },
		{ "RedrawAction", GTK_STOCK_REFRESH, "Redraw", "<control>R", 0, G_CALLBACK(do_refresh_plot) },
		{ "MoreIterationsAction", GTK_STOCK_EXECUTE, "More iterations", "<control>M", 0, G_CALLBACK(do_plot_more) },

	};

	static GtkToggleActionEntry toggles[] = {
		{ "DrawHUDAction", 0, "Draw _HUD", "<control>H", 0, G_CALLBACK(toggle_hud), render_ctx.draw_hud },
		{ "AntiAliasAction", 0, "_AntiAlias", "<control>A", 0, G_CALLBACK(toggle_antialias), render_ctx.antialias },
	};

	GtkActionGroup *action_group = gtk_action_group_new ("MainActions");
	gtk_action_group_add_actions(action_group, entries, G_N_ELEMENTS(entries), (void*)&gtk_ctx);
	gtk_action_group_add_toggle_actions(action_group, toggles, G_N_ELEMENTS(toggles), &gtk_ctx);

	gtk_ctx.mainctx = &render_ctx;

	GtkWidget *main_vbox, *menubar;
	GtkWidget *canvas;

	GtkWidget * window = gtk_ctx.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), 0);
	g_signal_connect(window, "destroy", G_CALLBACK(destroy_event), 0);

	gtk_window_set_title(GTK_WINDOW(window), "brot2"); // will be updated by renderer..
	main_vbox = gtk_vbox_new(FALSE,1);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	GError *err = 0;
	GtkUIManager *manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	guint uid = gtk_ui_manager_add_ui_from_string(manager, uixml, strlen(uixml), &err);
	if (!uid) {
		cerr << "Creating UI failed: " << err->message << endl;
		g_message("Creating UI failed: %s", err->message);
		g_error_free(err);
		return 1;
	}

	gtk_window_add_accel_group (GTK_WINDOW (gtk_ctx.window), gtk_ui_manager_get_accel_group(manager));

	menubar = gtk_ui_manager_get_widget (manager, "/menubar");

	ensure_Mandelbrot();
	ensure_Mandelbar();
	setup_fractal_menu(&gtk_ctx, menubar, "Mandelbrot");
	setup_colour_menu(&gtk_ctx, menubar, "Linear rainbow");

	canvas = gtk_drawing_area_new();
	gtk_widget_set_size_request (GTK_WIDGET(canvas), 300, 300);
	g_signal_connect (GTK_OBJECT (canvas), "expose_event",
			(GtkSignalFunc) expose_event, &gtk_ctx);
	g_signal_connect (GTK_OBJECT(canvas),"configure_event",
			(GtkSignalFunc) configure_event, &gtk_ctx);
	g_signal_connect (GTK_OBJECT (canvas), "motion_notify_event",
			(GtkSignalFunc) motion_notify_event, &gtk_ctx);
	g_signal_connect (GTK_OBJECT (canvas), "button_press_event",
			(GtkSignalFunc) button_press_event, &gtk_ctx);
	g_signal_connect (GTK_OBJECT (canvas), "button_release_event",
			(GtkSignalFunc) button_release_event, &gtk_ctx);
	g_signal_connect (GTK_OBJECT (window), "key_press_event",
			(GtkSignalFunc) key_press_event, &gtk_ctx);

	gtk_widget_set_events (canvas, GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);

	gtk_ctx.progressbar = GTK_PROGRESS_BAR(gtk_progress_bar_new());

	gtk_progress_bar_set_text(gtk_ctx.progressbar, "Initialising");
	gtk_progress_bar_set_ellipsize(gtk_ctx.progressbar, PANGO_ELLIPSIZE_END);
	gtk_progress_bar_set_pulse_step(gtk_ctx.progressbar, 0.1);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), canvas, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_vbox), GTK_WIDGET(gtk_ctx.progressbar), FALSE, FALSE, 0);

	render_ctx.initializing = false;
	gtk_widget_show_all(window);
	gtk_main();
	gdk_threads_leave();
	// Do not delete the fractal before the plot has been stopped, otherwise
	// a crash is inevitable if the user quits mid-plot.
	if (render_ctx.plot) {
		render_ctx.plot->stop();
		render_ctx.plot->wait();
	}
	return 0;
}
