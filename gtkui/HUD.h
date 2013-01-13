/*
    HUD: Heads-Up Display for brot2
    Copyright (C) 2010-2012 Ross Younger

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

#ifndef HUD_H_
#define HUD_H_

#include "libbrot2/Plot3Plot.h"
#include "libbrot2/Prefs.h"
#include <gtkmm/window.h>
#include <gdkmm/color.h>
#include <cairomm/cairomm.h>
#include <mutex>
#include <memory>


class MainWindow;

class HUD {
protected:
	MainWindow &parent;
	Cairo::RefPtr<Cairo::Surface> surface;
	int last_drawn_w, last_drawn_h; // Last size we drew to
	std::mutex mux; // Protects all contents

	// Must hold the lock before calling.
	void clear_locked(Cairo::RefPtr<Cairo::Context> cr);

	void retrieve_prefs(std::shared_ptr<const BrotPrefs::Prefs> prefs,
			Gdk::Color& fg, Gdk::Color& bg, double& alpha,
			int& xpos, int& ypos, int& xright, int& fontsize);

	// Must hold the lock before calling.
	void ensure_surface_locked(const int rwidth, const int rheight);

	unsigned compute_layout_height(Glib::RefPtr<Pango::Layout> lyt);

public:
	HUD(MainWindow &w);

	void draw(Plot3::Plot3Plot* plot, const int rwidth, const int rheight);
	void erase();
	Cairo::RefPtr<Cairo::Surface> & get_surface() { return surface; }

	static const std::string font_name;
};

#endif /* HUD_H_ */
