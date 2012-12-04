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

#include "png.h" // must be first, see launchpad 218409
#include <memory>
#include <iostream>

#include <glibmm.h>
#include <gtkmm.h>
#include <X11/Xlib.h>

#include "config.h"
#include "gtkmain.h"
#include "MainWindow.h"

static bool do_version;

static int realmain(int argc, char**argv)
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

	mainwind->show_all();
	gmain->run();
	gdk_threads_leave();

	delete mainwind;
	return 0;
}

int main (int argc, char**argv)
{
	try {
		realmain(argc,argv);
	} catch (Exception& e) {
		std::cerr << "Died from an uncaught exception. This shouldn't happen. Please debug. Message follows." << std::endl << e.msg << std::endl;
	}
}
