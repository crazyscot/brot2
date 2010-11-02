#include <gtk/gtk.h>
#include <sys/time.h>
#include <stdlib.h>
#include "mandel.h"

static GdkPixmap *render;
static GtkStatusbar *statusbar;

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

static void destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

static GtkItemFactoryEntry menu_items[] = {
	{ "/_Main", 0, 0, 0, "<Branch>" },
	{ "/Main/_Quit", "<control>Q", gtk_main_quit, 0, "<StockItem>" },
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

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event)
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
	draw_set(render, widget->style->white_gc,
			0, 0, widget->allocation.width, widget->allocation.height);
	gettimeofday(&tv_after,0);

	tv_diff = tv_subtract(tv_after, tv_before);
	double timetaken = tv_diff.tv_sec + (tv_diff.tv_usec / 1e6);

	gtk_statusbar_pop(statusbar, 0);
	const char * info = get_set_info();
	char * full_info = 0;
	asprintf(&full_info, "%s; render time was %lf", info, timetaken);
	gtk_statusbar_push(statusbar, 0, full_info);
	free((char*)info);
	free(full_info);
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

static gboolean button_press_event( GtkWidget *widget, GdkEventButton *event )
{
	if (event->button == 1 && render != NULL)
		/* do something! */ ;

	return TRUE;
}

static gboolean motion_notify_event( GtkWidget *widget, GdkEventMotion *event )
{
	int x, y;
	GdkModifierType state;

	if (event->is_hint)
		gdk_window_get_pointer (event->window, &x, &y, &state);
	else
	{
		x = event->x;
		y = event->y;
		state = event->state;
	}

	if (state & GDK_BUTTON1_MASK && render != NULL)
		/* do something! */;

	return TRUE;
}

int main (int argc, char**argv)
{
	GtkWidget *window, *main_vbox, *menubar;
	GtkWidget *canvas;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), 0);
	g_signal_connect(window, "destroy", G_CALLBACK(destroy), 0);

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

	gtk_widget_set_events (canvas, GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
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
	return 0;
}
