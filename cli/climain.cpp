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
#include <string>
#include <stdio.h>
#include <string.h>

#include <glibmm.h>
#include <X11/Xlib.h>

#include "config.h"
#include "climain.h"
#include "Plot2.h"
#include "palette.h"
#include "Fractal.h"
#include "reporter.h"
#include "Render.h"

static bool do_version;
static Glib::ustring c_re_x, c_im_y, length_x;

#define OPTION(_SHRT, _LNG, _DESC, _VAR) do {	\
	Glib::OptionEntry _t;						\
	_t.set_short_name(_SHRT);					\
	_t.set_long_name(_LNG);						\
	_t.set_description(_DESC);					\
	options.add_entry(_t, _VAR);				\
} while(0)

static void setup_options(Glib::OptionGroup& options)
{
	OPTION('v', "version", "Outputs this program's version number", do_version);
	OPTION('X', "real-centre", "Sets the Real (X) centre of the plot", c_re_x);
	OPTION('Y', "imaginary-centre", "Sets the Imaginary (Y) centre of the plot", c_im_y);
	OPTION('l', "real-axis-length", "Sets the length of the real (X) axis", length_x);
}

// returns false on error
static bool parse_fractal_value(Glib::ustring& in, Fractal::Value& out)
{
	std::istringstream tmp(in, std::istringstream::in);
	tmp.precision(MAXIMAL_DECIMAL_PRECISION);
	tmp >> out;
	if (tmp.fail()) {
		return false;
	}
	return true;
}

int main (int argc, char**argv)
{
	Glib::thread_init();

	Glib::OptionContext octx;
	Glib::OptionGroup options("brot2", "brot2 options", "Options relating to brot2");
	setup_options(options);

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

	bool fail=false;
	if (c_re_x.length()==0) {
		std::cerr << "Error: Real (X) centre is mandatory" << std::endl;
		fail=true;
	}
	if (c_im_y.length()==0) {
		std::cerr << "Error: Imaginary (Y) centre is mandatory" << std::endl;
		fail=true;
	}
	if (length_x.length()==0) {
		std::cerr << "Error: Axis length is mandatory" << std::endl;
		fail=true;
	}
	if (fail) return 4;

	Fractal::Value CRe, CIm, XAxisLength;
	if (!parse_fractal_value(c_re_x, CRe)) {
		std::cerr << "cannot parse input real centre " << c_re_x << std::endl;
		fail = true;
	}
	if (!parse_fractal_value(c_im_y, CIm)) {
		std::cerr << "cannot parse input imaginary centre " << c_im_y << std::endl;
		fail = true;
	}
	if (!parse_fractal_value(length_x, XAxisLength)) {
		std::cerr << "cannot parse input axis length " << length_x << std::endl;
		fail = true;
	}
	if (XAxisLength < MINIMUM_PIXEL_SIZE) {
		std::cerr << "input axis length is smaller than the resolution limit" << std::endl;
		fail = true;
	}

	if (fail) return 4;

	// XXX We are here: CLI args:
	// Fractal type (--list-fractals) (--fractal Mandelbrot) (ideally may abbreviate these, or specify by index no. in the list)
	// Palette (--list-palettes) (--palette "Linear rainbow") (also abbrev or index)
	// Render size (--height N --width N)
	// Progress (may be disabled, --> silent Reporter) (--quiet) (--no-progress)
	// Output filename (PNG only, at least for now) (--output foo.png) (MANDATORY)
	// Antialias option (default on?) -- double the plot pixels, set factor=2 (--antialias)

	std::string entered_fractal = "Mandelbrot"; // To become a parameter
	std::string entered_palette = "Logarithmic rainbow"; // To become a parameter
	Fractal::Point centre(CRe, CIm);
	const unsigned plot_h=1000, plot_w=1000, output_h=1000, output_w=1000, antialias=1;
	const char *filename = "brot2-out.png";

	double aspect = (double)plot_w / plot_h;
	Fractal::Value YAxisLength = XAxisLength / aspect;
	Fractal::Point size(XAxisLength, YAxisLength);

	// TODO allow pixel size / axes length to be specified in other ways

	Fractal::FractalCommon::load_base();
	Fractal::FractalImpl *selected_fractal = Fractal::FractalCommon::registry.get(entered_fractal);

	if (!selected_fractal) {
		std::cerr << "Fractal " << entered_fractal << " not found" << std::endl;
		return 5;
	}

	DiscretePalette::register_base();
	SmoothPalette::register_base();
	BasePalette *selected_palette = DiscretePalette::all.get(entered_palette);
	if (!selected_palette)
		selected_palette = SmoothPalette::all.get(entered_palette);
	if (!selected_palette) {
		std::cerr << "Palette " << entered_palette << " not found" << std::endl;
		return 5;
	}


	Plot2 plot(selected_fractal, centre, size, plot_w, plot_h);
	Reporter reporter;
	plot.start(&reporter);
	plot.wait();

	FILE *f = fopen(filename, "wb");
	if (!f) {
		std::cerr << "Could not open file for writing: " << errno << " (" << strerror(errno) << ")";
		return 4;
	}

	const char *err_str = "";
	if (0 != Render::save_as_png(f,
				output_w, output_h, plot, *selected_palette, antialias,
				&err_str)) {
		std::cerr << "Saving as PNG failed: " << err_str << std::endl;
		return 3;
	}

	if (0!=fclose(f)) {
		std::cerr << "Error closing the save: " << errno << " (" << strerror(errno) << ")";
		return 2;
	}

	// TODO: max iteration control?
	// allow HUD to be rendered? tricky.
	// XXX ensure built correctly into the deb.
	// XXX will need a manpage.
	return 0;
}
