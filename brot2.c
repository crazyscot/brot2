#include <gtk/gtk.h>
#include "mandel.h"

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

static GdkPixmap *render;

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
	if (render)
		g_object_unref(render);
	render = gdk_pixmap_new(widget->window,
			widget->allocation.width,
			widget->allocation.height,
			-1);
	draw_set(render, widget->style->white_gc,
			0, 0, widget->allocation.width, widget->allocation.height);
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

static void paint_it(GtkWidget *widget, gdouble xx, gdouble yy)
{
	GdkRectangle update_rect;

	update_rect.x = xx;
	update_rect.y = yy;
	update_rect.width = 10;
	update_rect.height = 10;
	gdk_draw_rectangle (render,
			widget->style->black_gc,
			TRUE,
			update_rect.x, update_rect.y,
			update_rect.width, update_rect.height);
	gtk_widget_queue_draw_area (widget, 		      
			update_rect.x, update_rect.y,
			update_rect.width, update_rect.height);

}

static gboolean button_press_event( GtkWidget *widget, GdkEventButton *event )
{
	if (event->button == 1 && render != NULL)
		paint_it(widget, event->x, event->y);

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
		paint_it(widget, x, y);

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

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), canvas, TRUE, TRUE, 0);

	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}
