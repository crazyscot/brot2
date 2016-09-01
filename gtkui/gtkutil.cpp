/*
    gtkutil.cpp: General GTK miscellanea that didn't fit anywhere else
    Copyright (C) 2011-6 Ross Younger

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

#include "gtkutil.h"
#include <gdkmm/screen.h>

namespace Util {

void get_screen_geometry(const Gtk::Window& window, int& x, int& y)
{
	auto screen = window.get_screen();
	x = screen->get_width();
	y = screen->get_height();
}

void fix_window_coords(const Gtk::Window& window, int& x, int& y)
{
	auto screen = window.get_screen();
	int width = screen->get_width(),
		height = screen->get_height();
	x = MAX(x, 0);
	x = MIN(x, width);
	y = MAX(y, 0);
	y = MIN(y, height);
}

}; // Util
