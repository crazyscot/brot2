/*
    SaveAsPNG: PNG export action/support for brot2
    Copyright (C) 2011-12 Ross Younger

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

#ifndef SAVEASPNG_H_
#define SAVEASPNG_H_

class MainWindow;
#include "misc.h"
BROT2_GTKMM_BEFORE
BROT2_GLIBMM_BEFORE
#include <gtkmm/progressbar.h>
#include <gtkmm/window.h>
BROT2_GLIBMM_AFTER
BROT2_GTKMM_AFTER

#include "Plot3Plot.h"
#include "Plot3Chunk.h"
#include "IPlot3DataSink.h"
#include "palette.h"
#include "ChunkDivider.h"
#include <string>
#include <memory>

namespace SavePNG {

class Single;

struct SingleProgressWindow: public Gtk::Window, Plot3::IPlot3DataSink {
	MainWindow& parent;
	Single& job;
	Gtk::ProgressBar *progbar;
	int _chunks_this_pass;
	SingleProgressWindow(MainWindow& p, Single& j);
	virtual void chunk_done(Plot3::Plot3Chunk* chunk);
	virtual void pass_complete(std::string& commentary, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels);
	virtual void plot_complete();
};

class Base {
	public:
		static std::string last_saved_dirname;
		static std::string default_save_dir(void);
		static void update_save_dir(const std::string& filename);

		void start(); // ->plot.start()
		void wait(); // -> plot.wait()
		void save_png(Gtk::Window *parent);
		unsigned get_chunks_count() const { return plot.chunks_total(); }

		static void to_png(Gtk::Window *parent, unsigned rwidth, unsigned rheight,
				Plot3::Plot3Plot* plot, const BasePalette* pal, bool antialias,
				bool show_hud, std::string& filename, bool upscale=false);
	protected:
		Base(std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> threads,
				const Fractal::FractalImpl& fractal, const BasePalette& palette,
				Plot3::IPlot3DataSink& sink,
				Fractal::Point centre, Fractal::Point size,
				unsigned width, unsigned height, bool antialias, bool do_hud, std::string&name, bool upscale=false);

		std::shared_ptr<const BrotPrefs::Prefs> prefs;
		std::shared_ptr<Plot3::ChunkDivider::Base> divider;
		const int aafactor, upfactor;
		Plot3::Plot3Plot plot;
		const BasePalette *pal; // do NOT delete
		std::string filename;
		const unsigned _width, _height;
		const bool _do_antialias, _do_hud, _upscale;

		virtual ~Base();
};

class Single : Base {
	// Interface for MainWindow to trigger save actions.
	// An instance of this class is an outstanding PNG-save job.
	friend class ::MainWindow;
	friend class SingleProgressWindow;

	// Private constructor! Called by do_save().
	Single(MainWindow* mw, Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, bool antialias, bool do_hud, std::string&name);

private:
	SingleProgressWindow reporter;

public:
	// main entrypoint: runs the save dialog and DTRTs
	static void do_save(MainWindow *mw);

	virtual ~Single();
};


class MovieFrame : public Base {
	public:
		MovieFrame(std::shared_ptr<const BrotPrefs::Prefs> prefs, std::shared_ptr<ThreadPool> threads, const Fractal::FractalImpl& fractal, const BasePalette& palette, Plot3::IPlot3DataSink& sink, Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, bool antialias, bool do_hud, std::string& name, bool upscale);
		virtual ~MovieFrame();
};

}; // namespace SavePNG

#endif /* SAVEASPNG_H_ */
