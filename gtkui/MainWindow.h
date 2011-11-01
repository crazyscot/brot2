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
#include "MouseHelp.h"
#include "Prefs.h"
#include "Menus.h"

#include <iostream>
#include <gtkmm/window.h>
#include <gtkmm/menubar.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/progressbar.h>
#include <sys/time.h>

class HUD;

class MainWindow : public Gtk::Window, Plot2::callback_t {
	Gtk::VBox *vbox; // Main layout widget
	menus::Menus *menubar;
	Canvas *canvas;
	Gtk::ProgressBar *progbar;
	HUD hud;
	MouseHelpWindow mousehelp;

	unsigned char *imgbuf;

	Plot2 * plot;
	Plot2 * plot_prev;

	Fractal::Point centre, size;
	unsigned rwidth, rheight; // Rendering dimensions; plot dims will be larger if antialiased
	bool draw_hud, antialias;
	unsigned antialias_factor;
	bool initializing; // Disables certain event actions when set.

	bool aspectfix, clip; // Details about the current render

	struct timeval plot_tv_start;

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

	const Fractal::Point& get_centre() const { return centre; }
	const Fractal::Point& get_size() const { return size; }

	void update_params(Fractal::Point& centre, Fractal::Point& size);
	void new_centre_checked(const Fractal::Point& centre);

    void do_zoom(enum Zoom z);
    void do_zoom(enum Zoom z, const Fractal::Point& newcentre);

    void render_cairo(int local_inf=-1);
    void recolour();
    void do_undo();
	void do_stop();
	void do_more_iters();

	inline bool hud_active() const {
		return draw_hud;
	}
	void toggle_hud();
	void toggle_antialias();
	int get_antialias() const {
		return antialias ? antialias_factor : 1;
	}

	Cairo::RefPtr<Cairo::Surface>& get_hud_surface() {
		return hud.get_surface();
	}

	MouseHelpWindow& mouseHelp() {
		return mousehelp;
	}

	Prefs& prefs() {
		return Prefs::getDefaultInstance();
	}

	menus::AbstractOptionsMenu *optionsMenu() {
		return menubar->optionsMenu;
	}

    // Plot2::callback_t:
	virtual void plot_progress_minor(Plot2& plot, float workdone);
	virtual void plot_progress_major(Plot2& plot, unsigned current_maxiter, std::string& commentary);
	virtual void plot_progress_complete(Plot2& plot);

private:
    void zoom_mechanics(enum Zoom z);

};

#endif /* MAINWINDOW_H_ */
