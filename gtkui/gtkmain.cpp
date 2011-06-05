/* main.cpp: main GTK program for brot2, license text follows */
const char* license_text = "\
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

const char *copyright_string = "(c) 2010-2011 Ross Younger";

#include <memory>
#include <iostream>

#include <glibmm.h>
#include <gtkmm.h>
#include <X11/Xlib.h>

#include "config.h"

static bool do_version;

int main (int argc, char**argv)
{
	XInitThreads();
	Glib::thread_init();

	gdk_threads_init(); // TODO See if these are still needed.
	gdk_threads_enter();

	Glib::OptionContext octx;
	Glib::OptionGroup options("brot2", "brot2 options", "Options relating to brot2");
	Glib::OptionEntry version;
	version.set_long_name("version");
	version.set_short_name('v');
	version.set_description("Outputs this program's version number");
	options.add_entry(version, do_version);

	octx.set_help_enabled(true);
	octx.set_ignore_unknown_options(false);
	octx.set_main_group(options);

	std::auto_ptr<Gtk::Main> gmain;
	try {
		gmain.reset(new Gtk::Main(argc, argv, octx));
	} catch(Glib::Exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	if (do_version) {
		std::cout << PACKAGE_STRING << std::endl;
		return EXIT_SUCCESS;
	}

#if 0
	// Initial settings (set up BEFORE the menubar):
	render_ctx.centre = { 0.0, 0.0 };
	render_ctx.size = { 4.5, 4.5 };

	render_ctx.draw_hud = true;
	render_ctx.antialias = false;
	// _main_ctx.pal initial setting by setup_colour_menu().
	// render_ctx.fractal set by setup_fractal_menu().

	// setup ACTION ENTRIES here.
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
#endif
	return 0;
}
