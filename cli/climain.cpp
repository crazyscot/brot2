/*
    climain.cpp: main GTK program for CLI (non-gtk) brot2
    Copyright (C) 2010-6 Ross Younger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <memory>
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include "misc.h"
BROT2_GLIBMM_BEFORE
#include <glibmm.h>
BROT2_GLIBMM_AFTER
#include <X11/Xlib.h>

#include "config.h"
#include "license.h"
#include "libbrot2/Plot3Plot.h"
#include "libbrot2/ChunkDivider.h"
#include "libbrot2/palette.h"
#include "libfractal/Fractal.h"
#include "CLIDataSink.h"
#include "libbrot2/Render2.h"
#include "libbrot2/Prefs.h"
#include "libbrot2/PrefsRegistry.h"
#include "libbrot2/BaseHUD.h"

using namespace Plot3;
using namespace BrotPrefs;

static bool do_version, do_license, do_list_fractals, do_list_palettes, quiet, do_antialias, do_csv, do_info, do_hud, do_upscale;
static Glib::ustring c_re_x, c_im_y, length_x;
static Glib::ustring entered_fractal = "Mandelbrot";
static Glib::ustring entered_palette = "Linear rainbow";
static Glib::ustring filename;
static int output_h=300, output_w=300, max_passes=0,
		   init_maxiter=-1, min_escapee_pct=-1;
static double live_threshold_fract=-1.0;

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
	OPTION('H', "hud", "Renders the HUD into the output PNG, using the current preferences", do_hud);

	OPTION('m', "max-passes", "Limits the number of passes of the plot", max_passes);
	OPTION('I', "initial-maxiter",
			PREFDESC(InitialMaxIter), init_maxiter);
	OPTION('E', "minimum-escapee-percent",
			PREFDESC(MinEscapeePct), min_escapee_pct);
	OPTION('T', "live-threshold-proportion",
			PREFDESC(LiveThreshold), live_threshold_fract);

	OPTION('q', "quiet", "Inhibits progress reporting", quiet);
	OPTION('a', "antialias", "Enables linear antialiasing", do_antialias);
	OPTION(0,   "csv", "Outputs as a CSV file", do_csv);
	OPTION(0,   "upscale", "Upscales the output by a factor of 2", do_upscale);

	OPTION('i', "info", "Outputs the plot's info string on completion", do_info);
	OPTION('v', "version", "Outputs this program's version number", do_version);
	OPTION(0,   "license", "Outputs this program's license information", do_license);
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
	if (do_license) {
		std::cout << brot2_license_text << std::endl;
		did_something=true;
	}
	if (do_version && !do_license) {
		std::cout << PACKAGE_STRING << " " << brot2_copyright_string << std::endl;
		std::cout << "To see the license for this software, run " << argv[0] << " --license" << std::endl;
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
	if (XAxisLength < Fractal::Maths::smallest_min_pixel_size()) {
		std::cerr << "input axis length is smaller than the resolution limit" << std::endl;
		fail = true;
	}
	if (filename.length()==0) {
		std::cerr << "output filename is required (use '-' for stdout)" << std::endl;
		fail = true;
	}
	if (do_csv && do_upscale) {
		std::cerr << "ERROR: --csv and --upscale are incompatible" << std::endl;
		fail = true;
	}
	if (do_antialias && do_upscale) {
		std::cerr << "ERROR: --antialias and --upscale are incompatible" << std::endl;
		fail = true;
	}
	if (fail) return 4;

	std::shared_ptr<const Prefs> mprefs = Prefs::getMaster();
	std::shared_ptr<Prefs> prefs = mprefs->getWorkingCopy();

	if (init_maxiter !=-1) {
		if (init_maxiter < PREF(InitialMaxIter)._min) {
			std::cerr << "Error: First pass maxiter (-I) must be at least 2" << std::endl;
			fail=true;
		} else {
			prefs->set(PREF(InitialMaxIter), init_maxiter);
		}
	}
	if (min_escapee_pct!=-1) {
		if ((min_escapee_pct<PREF(MinEscapeePct)._min) || (min_escapee_pct>PREF(MinEscapeePct)._max)) {
			std::cerr << "Error: Minimum escapee percent (-E) must be from 0 to 100" << std::endl;
			fail=true;
		} else {
			prefs->set(PREF(MinEscapeePct), min_escapee_pct);
		}
	}
	if (live_threshold_fract!=-1.0) {
		if ((live_threshold_fract<PREF(LiveThreshold)._min) || (live_threshold_fract>PREF(LiveThreshold)._max)) {
			std::cerr << "Error: Pixel escape maximum speed (-T) must be between 0.0 and 1.0" << std::endl;
			fail=true;
		}
		else {
			prefs->set(PREF(LiveThreshold), live_threshold_fract);
		}
	}
	if (fail) return 4;

	Fractal::Point centre(CRe, CIm);
	const unsigned antialias= do_antialias ? 2 : 1;
	unsigned plot_h=output_h*antialias, plot_w=output_w*antialias;
	if (do_upscale) {
		plot_h /= 2;
		plot_w /= 2;
	}

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

	bool do_stdout = false;
	if (filename.length()==1 && filename[0]=='-') {
		do_stdout = true;
	}

	int nthreads = BrotPrefs::threadpool_size(prefs);

	CLIDataSink sink(0, quiet);
	std::shared_ptr<ThreadPool> pool(new ThreadPool(nthreads));
	ChunkDivider::Horizontal10px divider;
	Plot3Plot plot(pool, &sink, *selected_fractal, divider,
			centre, size, plot_w, plot_h, max_passes);

	sink.set_plot(&plot);
	plot.set_prefs(prefs);

	try {
		plot.start();
	} catch (BrotException &e) {
		// Usually means the pixels are too small for all known types.
		std::cerr << "Plot failed to start: " << e.msg << std::endl;
		return 4;
	}
	plot.wait();
	ASSERT(sink.is_done());
	if (!quiet)
		std::cerr << std::endl << "Complete!" << std::endl;

	Render2::Writable * render = 0;

	if (do_csv) {
		render = new Render2::CSV(output_w, output_h, *selected_palette, -1, do_antialias);
	} else {
		render = new Render2::PNG(output_w, output_h, *selected_palette, -1, do_antialias, do_upscale);
	}

	for (auto it : sink.get_chunks_done())
		render->process(*it);
	if (do_hud)
		BaseHUD::apply(*render, prefs, &plot, false, false);
	if (do_stdout)
		render->write(std::cout);
	else
		render->write(filename);
	delete render;

	if (do_info)
		std::cout << plot.info(true) << std::endl;
	return 0;
}
