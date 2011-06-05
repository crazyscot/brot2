/* gtkmain.cpp: main GTK program for brot2, license text follows */
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
#include "gtkmain.h"
#include "MainWindow.h"

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

	MainWindow *mainwind = new MainWindow();

	// XXX put a Plot2 into MainWindow (two of them?)
	// 		- set up default params

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
#endif

	mainwind->show_all();
	gmain->run();
	gdk_threads_leave();

#if 0
	// Do not delete the fractal before the plot has been stopped, otherwise
	// a crash is inevitable if the user quits mid-plot.
	if (render_ctx.plot) {
		render_ctx.plot->stop();
		render_ctx.plot->wait();
	}
#endif
	return 0;
}
