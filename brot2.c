#include <gtk/gtk.h>

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_print("delete event\n");
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


int main (int argc, char**argv)
{
	GtkWidget *window, *main_vbox;
	GtkWidget *menubar;
	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), 0);
	g_signal_connect(window, "destroy", G_CALLBACK(destroy), 0);

	gtk_window_set_title(GTK_WINDOW(window), "brot2");
	gtk_widget_set_size_request (GTK_WIDGET(window), 640, 480);
	main_vbox = gtk_vbox_new(FALSE,1);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	menubar = get_menubar_menu(window);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, TRUE, 0);

	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}
