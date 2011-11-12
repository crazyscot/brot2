/*
    HUD: Heads-Up Display for brot2
    Copyright (C) 2010-2011 Ross Younger

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

#include "Plot2.h"
#include <gtkmm/window.h>
#include <cairomm/cairomm.h>
#include <glibmm/thread.h>

class MainWindow;

class HUD {
protected:
	MainWindow &parent;
	Cairo::RefPtr<Cairo::Surface> surface;
	int w, h; // Last size we drew to

	void clear_locked(Cairo::RefPtr<Cairo::Context> cr);

	Glib::Mutex mux; // Protects all contents

public:
	HUD(MainWindow &w);

	void draw(Plot2* plot, const int rwidth, const int rheight);
	Cairo::RefPtr<Cairo::Surface> & get_surface() { return surface; }

	static const std::string font_name;
};

#endif /* HUD_H_ */
