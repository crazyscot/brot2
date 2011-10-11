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

static bool do_version, do_list_fractals, do_list_palettes, quiet, do_antialias, do_info;
static Glib::ustring c_re_x, c_im_y, length_x;
static Glib::ustring entered_fractal = "Mandelbrot";
static Glib::ustring entered_palette = "Linear rainbow";
static Glib::ustring filename;
static int output_h=300, output_w=300, max_passes=0;

#define OPTION(_SHRT, _LNG, _DESC, _VAR) do {	\
	Glib::OptionEntry _t;						\
	_t.set_short_name(_SHRT);					\
	_t.set_long_name(_LNG);						\
	_t.set_description(_DESC);					\
	options.add_entry(_t, _VAR);				\
} while(0)

static void setup_options(Glib::OptionGroup& options)
{
	OPTION('X', "real-centre", "Sets the Real (X) centre of the plot", c_re_x);
	OPTION('Y', "imaginary-centre", "Sets the Imaginary (Y) centre of the plot", c_im_y);
	OPTION('l', "real-axis-length", "Sets the length of the real (X) axis", length_x);

	OPTION('f', "fractal", "The fractal to use", entered_fractal);
	OPTION(0, "list-fractals", "Lists all known fractals", do_list_fractals);
	OPTION('p', "palette", "The palette to use", entered_palette);
	OPTION(0, "list-palettes", "Lists all known palettes", do_list_palettes);

	OPTION('h', "height", "Height of the output in pixels", output_h);
	OPTION('w', "width", "Width of the output in pixels", output_w);

	OPTION('o', "output", "The filename to write to (or '-' for stdout)", filename);

	OPTION('m', "max-passes", "Limits the number of passes of the plot", max_passes);

	OPTION('q', "quiet", "Inhibits progress reporting", quiet);
	OPTION('a', "antialias", "Enables linear antialiasing", do_antialias);

	OPTION('i', "info", "Outputs the plot's info string on completion", do_info);
	OPTION('v', "version", "Outputs this program's version number", do_version);
}

// returns false on error
static bool parse_fractal_value(Glib::ustring& in, Fractal::Value& out)
{
	std::istringstream tmp(in, std::istringstream::in);
	tmp >> out;
	if (tmp.fail()) {
		return false;
	}
	return true;
}

static void list_fractals(void)
{
	std::set<std::string> names = Fractal::FractalCommon::registry.names();
	std::set<std::string>::iterator it;

	std::cout << "Fractals:" << std::endl;
	for (it = names.begin(); it != names.end(); it++)
		std::cout << '\t' << *it << std::endl;
	std::cout << std::endl;
}

static void list_palettes(void)
{
	std::set<std::string>::iterator it;

	std::cout << "Palettes:" << std::endl;
	std::cout << "  (key: [D] Discrete, [S] Smooth)" << std::endl;

	std::set<std::string> names = DiscretePalette::all.names();
	for (it = names.begin(); it != names.end(); it++)
		std::cout << "   [D]\t" << *it << std::endl;
	names = SmoothPalette::all.names();
	for (it = names.begin(); it != names.end(); it++)
		std::cout << "   [S]\t" << *it << std::endl;
	std::cout << std::endl;
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

	Fractal::FractalCommon::load_base();
	DiscretePalette::register_base();
	SmoothPalette::register_base();

	bool did_something=false;
	if (do_version) {
		std::cout << PACKAGE_STRING << std::endl;
		did_something=true;
	}
	if (do_list_fractals) {
		list_fractals();
		did_something=true;
	}
	if (do_list_palettes) {
		list_palettes();
		did_something=true;
	}
	if (did_something) return EXIT_SUCCESS;

	bool fail=false;
	if (c_re_x.length()==0) {
		std::cerr << "Error: Real centre (-X) is mandatory" << std::endl;
		fail=true;
	}
	if (c_im_y.length()==0) {
		std::cerr << "Error: Imaginary centre (-Y) is mandatory" << std::endl;
		fail=true;
	}
	if (length_x.length()==0) {
		std::cerr << "Error: Axis length (-l) is mandatory" << std::endl;
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
	if (filename.length()==0) {
		std::cerr << "output filename is required (use '-' for stdout)" << std::endl;
		fail = true;
	}
	if (fail) return 4;

	Fractal::Point centre(CRe, CIm);
	const unsigned antialias= do_antialias ? 2 : 1;
	unsigned plot_h=output_h*antialias, plot_w=output_w*antialias;

	double aspect = (double)plot_w / plot_h;
	Fractal::Value YAxisLength = XAxisLength / aspect;
	Fractal::Point size(XAxisLength, YAxisLength);

	// TODO allow pixel size / axes length to be specified in other ways

	Fractal::FractalImpl *selected_fractal = Fractal::FractalCommon::registry.get(entered_fractal);

	if (!selected_fractal) {
		std::cerr << "Fractal " << entered_fractal << " not found" << std::endl;
		return 5;
	}

	BasePalette *selected_palette = DiscretePalette::all.get(entered_palette);
	if (!selected_palette)
		selected_palette = SmoothPalette::all.get(entered_palette);
	if (!selected_palette) {
		std::cerr << "Palette " << entered_palette << " not found" << std::endl;
		return 5;
	}

	FILE *closeme=0, *f;
	if (filename.length()==1 && filename[0]=='-') {
		f = stdout;
	} else {
		f = fopen(filename.c_str(), "wb");
		closeme = f;
	}
	if (!f) {
		std::cerr << "Could not open file for writing: " << errno << " (" << strerror(errno) << ")" << std::endl;
		return 4;
	}

	Plot2 plot(selected_fractal, centre, size, plot_w, plot_h, max_passes);
	Reporter reporter(0, quiet);
	plot.start(&reporter);
	plot.wait();

	const char *err_str = "";
	if (0 != Render::save_as_png(f,
				output_w, output_h, plot, *selected_palette, antialias,
				&err_str)) {
		std::cerr << "Saving as PNG failed: " << err_str << std::endl;
		return 3;
	}

	if (closeme && 0!=fclose(closeme)) {
		std::cerr << "Error closing the save: " << errno << " (" << strerror(errno) << ")";
		return 2;
	}

	if (do_info)
		std::cout << plot.info(true) << std::endl;

	// TODO: max iteration control?
	// allow HUD to be rendered? tricky.
	return 0;
}
