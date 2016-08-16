/*
    MovieMode: brot2 movie plotting
    Copyright (C) 2016 Ross Younger

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

#ifndef MOVIEMODE_H
#define MOVIEMODE_H

#include "Prefs.h"
#include "Fractal.h"
#include "FractalMaths.h"
#include "palette.h"
#include <vector>

namespace Movie {

struct Frame {
	Fractal::Point centre, size;
};

struct KeyFrame {
	Fractal::Point centre, size;
	unsigned hold_frames; // How many video frames to linger on this render
	unsigned frames_to_next; // How many video frames from this point to the next. Ignored unless there is another member in the points vector.
};

struct MovieInfo {
	const Fractal::FractalImpl * fractal;
	const BasePalette * palette;
	unsigned width, height;
	bool draw_hud, antialias;
	unsigned fps;
	std::vector<KeyFrame> points;

	MovieInfo() : fractal(0), palette(0), width(0), height(0), fps(0) {}
	unsigned count_frames() const;
};

}; // namespace Movie

#endif // MOVIEMODE_H
