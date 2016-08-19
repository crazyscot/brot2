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

struct KeyFrame {
	Fractal::Point centre, size;
	unsigned hold_frames; // How many video frames to linger on this render
	unsigned speed_zoom, speed_translate; // Speeds for motion to next key frame (ignored if this is last)
	KeyFrame(Fractal::Point _c, Fractal::Point _s, unsigned _h, unsigned _sz, unsigned _st) :
		centre(_c), size(_s), hold_frames(_h), speed_zoom(_sz), speed_translate(_st) {}
};

struct Frame {
	Fractal::Point centre, size;
	Frame() : centre(0,0), size(0,0) {}
	Frame(Fractal::Point _c, Fractal::Point _s) : centre(_c), size(_s) {}
	Frame(struct KeyFrame kf) : centre(kf.centre), size(kf.size) {}
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
