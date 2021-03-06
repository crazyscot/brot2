/*
    MainWindow: GTK+ (gtkmm) main window for brot2
    Copyright (C) 2011 Ross Younger

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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "Canvas.h"
#include "Plot3Plot.h"
#include "ChunkDivider.h"
#include "IPlot3DataSink.h"
#include "palette.h"
#include "Fractal.h"
#include "DragRectangle.h"
#include "HUD.h"
#include "ControlsWindow.h"
#include "MovieWindow.h"
#include "Prefs.h"
#include "Menus.h"
#include "Render2.h"
#include "libbrot2/ThreadPool.h"
#include "SaveAsPNG.h"

#include <iostream>
#include <memory>
#include <set>
BROT2_GTKMM_BEFORE
#include <gtkmm/window.h>
#include <gtkmm/menubar.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/progressbar.h>
BROT2_GTKMM_AFTER
#include <sys/time.h>

class HUD;
class ControlsWindow;
class MovieWindow;

class MainWindow : public Gtk::Window, Plot3::IPlot3DataSink {
	Gtk::VBox *vbox; // Main layout widget
	menus::Menus *menubar;
	Canvas *canvas;
	Gtk::ProgressBar *progbar;
	HUD hud;
	ControlsWindow controlsWin;
	MovieWindow movieWin;

	unsigned char *imgbuf;

	Plot3::Plot3Plot * plot;
	Plot3::Plot3Plot * plot_prev;
	Render2::MemoryBuffer * renderer;

	Fractal::Point centre, size;
	unsigned rwidth, rheight; // Rendering dimensions; plot dims will be larger if antialiased
	bool draw_hud, antialias, fullscreen_requested;
	bool initializing; // Disables certain event actions when set.

	bool aspectfix, at_max_zoom, at_min_zoom, recolour_when_done; // Details about the current render

	struct timeval plot_tv_start;

	Plot3::ChunkDivider::Base* divider;
	std::atomic<int> _chunks_this_pass; // Reset to 0 on pass completion.

public:
	BasePalette * pal;
	// Yes, the following are mostly the same as in the Plot - but the plot may be torn down and recreated frequently.
	Fractal::FractalImpl *fractal;
	DragRectangle dragrect;

	enum Zoom {
		REDRAW_ONLY,
		ZOOM_IN,
		ZOOM_OUT,
	};
	static const double ZOOM_FACTOR;

	MainWindow();
	virtual ~MainWindow();

	// Not copyable or assignable.
	MainWindow(const MainWindow&) = delete;
	const MainWindow operator= (const MainWindow&) = delete;

	int get_rwidth() const { return rwidth; }
	int get_rheight() const { return rheight; }
	Plot3::Plot3Plot& get_plot() const { return *plot; }
	Gtk::ProgressBar* get_progbar() const { return progbar; }

    virtual bool on_key_release_event(GdkEventKey *);
    virtual bool on_delete_event(GdkEventAny *);
    bool do_quit();

    void do_resize(unsigned width, unsigned height);
    void do_plot(bool is_same_plot = false);
    void safe_stop_plot();

	const Fractal::Point& get_centre() const { return centre; }
	const Fractal::Point& get_size() const { return size; }
	bool is_at_max_zoom() const { return at_max_zoom; }
	bool is_at_min_zoom() const { return at_min_zoom; }
	bool is_aspect_fixed() const { return aspectfix; }

	void update_params(Fractal::Point& centre, Fractal::Point& size);
	void new_centre_checked(const Fractal::Point& centre, bool is_zoom);

    void do_zoom(enum Zoom z);
    void do_zoom(enum Zoom z, const Fractal::Point& newcentre);

    // Prepare to render. Sets up everything needed to start passing chunks in.
    void render_prep(int local_inf);

    // Call after processing a chunk.
    void render_buffer_updated(Plot3::Plot3Chunk *job);

    // Call after a plot is finished, will redraw the HUD if approprate
    void render_buffer_tidyup();

    // Pushes the render buffer out to the display. Optionally reprocesses all chunks (only use after the plot is complete, e.g. to change the palette). If a job is provided, only marks the relevant part of the window as dirty.
    void render(int local_inf, bool do_reprocess, bool may_do_hud, Plot3::Plot3Chunk *job = NULL);

    void recolour();
    void do_undo();
	void do_stop();
	void do_more_iters();
	void do_reset();

	inline bool hud_active() const {
		return draw_hud;
	}
	void toggle_hud();
	void toggle_antialias();
	void toggle_fullscreen();
	bool is_antialias() const { return antialias; }
	unsigned get_menubar_height();

	Cairo::RefPtr<Cairo::Surface>& get_hud_surface() {
		return hud.get_surface();
	}

	ControlsWindow& controlsWindow() {
		return controlsWin;
	}

	MovieWindow& movieWindow() {
		return movieWin;
	}

	std::shared_ptr<const BrotPrefs::Prefs> prefs() {
		return BrotPrefs::Prefs::getMaster();
	}

	menus::AbstractOptionsMenu *optionsMenu() {
		return menubar->optionsMenu;
	}

	void queue_png(std::shared_ptr<SavePNG::Single> png);

	// IPlot3DataSink:
	virtual void chunk_done(Plot3::Plot3Chunk* job);
	virtual void pass_complete(std::string& commentary, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels);
	virtual void plot_complete();

	static const unsigned DEFAULT_INITIAL_SIZE = 300;

private:
    void zoom_mechanics(enum Zoom z);
	bool on_timer();
	void png_save_completion();
	void destroy_image();
	void real_plot_complete();

	std::shared_ptr<ThreadPool> _threadpool;
public:
	std::shared_ptr<ThreadPool> get_threadpool(); // singleton-like accessor
	void resize_threadpool(unsigned max_threads); // Called when Prefs updated
};

#endif /* MAINWINDOW_H_ */
