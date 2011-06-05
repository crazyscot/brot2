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
#include "Plot2.h"
#include "palette.h"
#include "Fractal.h"
#include "DragRectangle.h"
#include "HUD.h"

#include <iostream>
#include <gtkmm/window.h>
#include <gtkmm/menubar.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/progressbar.h>
#include <sys/time.h>

class HUD;

class pixpack_format {
	// A pixel format identifier that supersets Cairo's.
	int f; // Pixel format - one of cairo_format_t or our internal constants
public:
	static const int PACKED_RGB_24 = CAIRO_FORMAT_RGB16_565 + 1000000;
	pixpack_format(int c): f(c) {};
	inline operator int() const { return f; }
};

class MainWindow : public Gtk::Window, Plot2::callback_t {
	friend class Canvas;

	Gtk::VBox *vbox; // Main layout widget
	Gtk::MenuBar *menubar;
	Canvas *canvas;
	Gtk::ProgressBar *progbar;
	HUD hud;
	DragRectangle dragrect;

	unsigned char *imgbuf;

	Plot2 * plot;
	Plot2 * plot_prev;
	BasePalette * pal;
	// Yes, the following are mostly the same as in the Plot - but the plot may be torn down and recreated frequently.
	Fractal::FractalImpl *fractal;
	Fractal::Point centre, size;
	unsigned rwidth, rheight; // Rendering dimensions; plot dims will be larger if antialiased
	bool draw_hud, antialias;
	unsigned antialias_factor;
	bool initializing; // Disables certain event actions when set.

	bool aspectfix, clip; // Details about the current render

	struct timeval plot_tv_start;


public:
	enum Zoom {
		REDRAW_ONLY,
		ZOOM_IN,
		ZOOM_OUT,
	};
	static const int DEFAULT_ANTIALIAS_FACTOR;
	static const double ZOOM_FACTOR;

	MainWindow();
	virtual ~MainWindow();

	int get_rwidth() const { return rwidth; }
	int get_rheight() const { return rheight; }
	Plot2& get_plot() const { return *plot; }
	Gtk::ProgressBar* get_progbar() const { return progbar; }

    virtual bool on_key_release_event(GdkEventKey *);
    virtual bool on_delete_event(GdkEventAny *);

    void do_resize(unsigned width, unsigned height);
    void do_plot(bool is_same_plot = false);
    void safe_stop_plot();

    void do_zoom(enum Zoom z);
    void render_cairo(int local_inf=-1);
    void recolour();

    bool render_generic(unsigned char *buf, const int rowstride, const int local_inf, pixpack_format fmt);

    // Renders a single pixel, given the current idea of infinity and the palette to use.
    static inline rgb render_pixel(const Fractal::PointData *data, const int local_inf, const BasePalette * pal) {
		if (data->iter == local_inf || data->iter<0) {
			return black;
		} else {
			return pal->get(*data);
		}
	}

    static const int RGB_BYTES_PER_PIXEL = 3;

    // Plot2::callback_t:
	virtual void plot_progress_minor(Plot2& plot, float workdone);
	virtual void plot_progress_major(Plot2& plot, unsigned current_maxiter, std::string& commentary);
	virtual void plot_progress_complete(Plot2& plot);
};

#endif /* MAINWINDOW_H_ */
