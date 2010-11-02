#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include "mandelbrot.h"

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

static GtkWidget *window;
static GdkPixmap *render;
static GtkStatusbar *statusbar;
static mandelbrot_ctx *main_ctx;

static void do_redraw(GtkWidget *widget);

static void update_entry_float(GtkWidget *entry, double val)
{
	char * tmp = 0;
	asprintf(&tmp, "%f", val);
	gtk_entry_set_text(GTK_ENTRY(entry), tmp);
	free(tmp);
}

static void read_entry_float(GtkWidget *entry, double *val_out)
{
	long double tmp;
	const gchar * raw = gtk_entry_get_text(GTK_ENTRY(entry));
	if (1 == sscanf(raw, "%Lf", &tmp)) {
		*val_out = tmp;
		// TODO: Better error handling. Perhaps a validity check at dialog run time?
	}
}

void configure_mandelbrot(mandelbrot_ctx *ctx)
{
	// TODO: generate from renderer parameters!
	GtkWidget *dlg = gtk_dialog_new_with_buttons("Configuration", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			NULL);
	GtkWidget * content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	GtkWidget * label;

	//label = gtk_label_new ("Configuration");
	//gtk_container_add (GTK_CONTAINER (content_area), label);

	GtkWidget *c_re, *c_im, *size_re, *size_im;
	c_re = gtk_entry_new();
	update_entry_float(c_re, ctx->centre.re);
	c_im = gtk_entry_new();
	update_entry_float(c_im, ctx->centre.im);
	size_re = gtk_entry_new();
	update_entry_float(size_re, ctx->size.re);
	size_im = gtk_entry_new();
	update_entry_float(size_im, ctx->size.im);

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
		read_entry_float(c_re, &ctx->centre.re);
		read_entry_float(c_im, &ctx->centre.im);
		read_entry_float(size_re, &ctx->size.re);
		read_entry_float(size_im, &ctx->size.im);
		do_redraw(window);
		// FIXME: This is a bit nasty, but will be fixed when renders go asynch.
	}
	gtk_widget_destroy(dlg);
}

void do_config(void)
{
	configure_mandelbrot(main_ctx);
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

static GtkItemFactoryEntry menu_items[] = {
	{ "/_Main", 0, 0, 0, "<Branch>" },
	{ "/Main/_Params", "<control>P", do_config, 0, "<Item>" },
	{ "/Main/_Quit", "<control>Q", gtk_main_quit, 0, "<Item>" },
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

/* Returns a menubar widget made from the above menu */
static GtkWidget *get_menubar_menu( GtkWidget  *window )
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
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

	/* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}

// Redraws us onto a given widget (window), then queues an expose event
static void do_redraw(GtkWidget *widget)
{
	struct timeval tv_before, tv_after, tv_diff;

	if (render)
		g_object_unref(render);
	render = gdk_pixmap_new(widget->window,
			widget->allocation.width,
			widget->allocation.height,
			-1);
	gtk_statusbar_pop(statusbar, 0);
	gtk_statusbar_push(statusbar, 0, "Drawing..."); // FIXME: Doesn't update. Possibly leave this until we get computation multithreaded and asynch?
	gettimeofday(&tv_before,0);
	mandelbrot_draw(main_ctx, render, widget->style->white_gc,
			0, 0, widget->allocation.width, widget->allocation.height);
	gettimeofday(&tv_after,0);

	tv_diff = tv_subtract(tv_after, tv_before);
	double timetaken = tv_diff.tv_sec + (tv_diff.tv_usec / 1e6);

	gtk_statusbar_pop(statusbar, 0);
	const char * info = mandelbrot_info(main_ctx);
	char * full_info = 0;
	asprintf(&full_info, "%s; render time was %lf", info, timetaken);
	gtk_statusbar_push(statusbar, 0, full_info);
	free((char*)info);
	free(full_info);

	gtk_widget_queue_draw(widget);
}

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
	do_redraw(widget);
	return TRUE;
}

static gboolean expose_event( GtkWidget *widget, GdkEventExpose *event )
{
	gdk_draw_drawable(widget->window,
			widget->style->fg_gc[gtk_widget_get_state (widget)],
			render,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);

	return FALSE;
}

static gint dragrect_origin_x, dragrect_origin_y,
			dragrect_active=0, dragrect_current_x, dragrect_current_y;

static gboolean button_press_event( GtkWidget *widget, GdkEventButton *event )
{
#define ZOOM_FACTOR 2.0f
	if (render != NULL) {
		int redraw=0;
		complexF new_ctr;
		if (1==mandelbrot_pixel_to_set(main_ctx, event->x, event->y, &new_ctr)) {
			//printf("button %d @ %d,%d -> %f,%f\n", event->button, (int)event->x, (int)event->y, new_ctr.re, new_ctr.im);
			if (event->button >= 1 && event->button <= 3) {
				main_ctx->centre.re = new_ctr.re;
				main_ctx->centre.im = new_ctr.im;
				switch(event->button) {
				case 1:
					// LEFT: zoom in a bit
					main_ctx->size.re /= ZOOM_FACTOR;
					main_ctx->size.im /= ZOOM_FACTOR;
					break;
				case 3:
					// RIGHT: zoom out
					main_ctx->size.re *= ZOOM_FACTOR;
					main_ctx->size.im *= ZOOM_FACTOR;
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
		}
		if (redraw)
			do_redraw(window); // TODO: asynch drawing
	}

	return TRUE;
}

static gboolean button_release_event( GtkWidget *widget, GdkEventButton *event )
{
	if (event->button != 8)
		return FALSE;

	//printf("button %d UP @ %d,%d\n", event->button, (int)event->x, (int)event->y);

	if (render != NULL) {
		int l = MIN(event->x, dragrect_origin_x);
		int r = MAX(event->x, dragrect_origin_x);
		int t = MIN(event->y, dragrect_origin_y);
		int b = MAX(event->y, dragrect_origin_y);

		// centres
		complexF TL, BR;
		if (0==mandelbrot_pixel_to_set(main_ctx, l, t, &TL)) {
			printf("argh, top left is outside of current render\n"); //XXX
			return FALSE;
		}
		if (0==mandelbrot_pixel_to_set(main_ctx, r, b, &BR)) {
			printf("argh, bottom right is outside of current render\n"); //XXX
			return FALSE;
		}
		main_ctx->centre.re = (TL.re + BR.re) / 2;
		main_ctx->centre.im = (TL.im + BR.im) / 2;
		main_ctx->size.re = BR.re - TL.re;
		main_ctx->size.im = BR.im - TL.im;

		dragrect_active = 0;

		do_redraw(window); // TODO: asynch drawing
	}
	return TRUE;
}

static void draw_rect(GtkWidget *widget, GdkPixmap *pix, int L, int R, int T, int B)
{
	gdk_gc_set_function(window->style->black_gc, GDK_INVERT);
	gdk_draw_line(pix, widget->style->black_gc, L, T, R, T);
	gdk_draw_line(pix, widget->style->black_gc, R, T, R, B);
	gdk_draw_line(pix, widget->style->black_gc, R, B, L, B);
	gdk_draw_line(pix, widget->style->black_gc, L, B, L, T);
}

static gboolean motion_notify_event( GtkWidget *widget, GdkEventMotion *event )
{
	int x, y;
	GdkModifierType state;

	if (!dragrect_active) return FALSE;

	if (event->is_hint)
		gdk_window_get_pointer (event->window, &x, &y, &state);
	else
	{
		x = event->x;
		y = event->y;
		state = event->state;
	}

	// turn off previous hilight rect
	draw_rect(widget, render, dragrect_origin_x, dragrect_current_x, dragrect_origin_y, dragrect_current_y);

	// turn on our new one
	draw_rect(widget, render, dragrect_origin_x, x, dragrect_origin_y, y);
	dragrect_current_x = x;
	dragrect_current_y = y;

	gtk_widget_queue_draw (widget);
	// TODO queue_draw_area() instead

	//printf("button %d @ %d,%d\n", state, x, y);//XXX
	return TRUE;
}

int main (int argc, char**argv)
{
	GtkWidget *main_vbox, *menubar;
	GtkWidget *canvas;

	main_ctx = mandelbrot_new();
	// TODO proper configury. Use default params of a drawer.
	main_ctx->centre.re = -0.7;
	main_ctx->centre.im = 0.0;
	main_ctx->size.re = 3.0;
	main_ctx->size.im = 3.0;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), 0);
	g_signal_connect(window, "destroy", G_CALLBACK(destroy_event), 0);

	gtk_window_set_title(GTK_WINDOW(window), "brot2");
	main_vbox = gtk_vbox_new(FALSE,1);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	menubar = get_menubar_menu(window);

	canvas = gtk_drawing_area_new();
	gtk_widget_set_size_request (GTK_WIDGET(canvas), 200, 200);
	gtk_signal_connect (GTK_OBJECT (canvas), "expose_event",
			(GtkSignalFunc) expose_event, NULL);
	gtk_signal_connect (GTK_OBJECT(canvas),"configure_event",
			(GtkSignalFunc) configure_event, NULL);
	gtk_signal_connect (GTK_OBJECT (canvas), "motion_notify_event",
			(GtkSignalFunc) motion_notify_event, NULL);
	gtk_signal_connect (GTK_OBJECT (canvas), "button_press_event",
			(GtkSignalFunc) button_press_event, NULL);
	gtk_signal_connect (GTK_OBJECT (canvas), "button_release_event",
			(GtkSignalFunc) button_release_event, NULL);

	gtk_widget_set_events (canvas, GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);

	statusbar = GTK_STATUSBAR(gtk_statusbar_new());
	gtk_statusbar_push(statusbar, 0, "Initialising");
	gtk_statusbar_set_has_resize_grip(statusbar, 0);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), canvas, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_vbox), GTK_WIDGET(statusbar), FALSE, FALSE, 0);

	gtk_widget_show_all(window);
	gtk_main();
	mandelbrot_destroy(&main_ctx);
	return 0;
}
