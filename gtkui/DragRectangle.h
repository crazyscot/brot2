/*
    DragRectangle: mouse-driven drag-out rectangle graphics/tracking
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

#ifndef DRAGRECTANGLE_H_
#define DRAGRECTANGLE_H_

#include "misc.h"
#include <cairomm/cairomm.h>
#include <valarray>

class MainWindow;

class DragRectangle {
protected:
	MainWindow &parent;
	Cairo::RefPtr<Cairo::Surface> surface;

	void clear(Cairo::RefPtr<Cairo::Context> cr);

	bool active;
	Util::xy origin, current;

	void draw_internal();

public:
	DragRectangle(MainWindow &w);

	bool is_active() const { return active; }
	Util::xy get_origin() const { return origin; }
	void activate(int x, int y); // x,y = origin TODO take a Util::xy ?
	void draw(); // Deactivates and clears the surface
	void draw(int x, int y); // Redraws the surface relative to the origin
	void resized(); // Call if the underlying window has been resized
	Cairo::RefPtr<Cairo::Surface> & get_surface() { return surface; }

	static const std::valarray<double> dashes;
};

#endif /* DRAGRECTANGLE_H_ */
