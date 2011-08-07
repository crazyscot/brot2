/* climain.cpp: main GTK program for non-gtk brot2, license text follows */
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
#include <X11/Xlib.h>

#include "config.h"
#include "climain.h"

static bool do_version;

int main (int argc, char**argv)
{
	Glib::thread_init();

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

	try {
		if (!octx.parse(argc,argv))
			return EXIT_FAILURE;
	} catch(Glib::Exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	if (do_version) {
		std::cout << PACKAGE_STRING << std::endl;
		return EXIT_SUCCESS;
	}

	// XXX Don't link in libgtk
	// XXX Go DTRT !

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
