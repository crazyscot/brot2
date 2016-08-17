/*
    MovieMotion: brot2 frame-by-frame motion for movie plotting
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

#ifndef MOVIEMOTION_H
#define MOVIEMOTION_H

#include "MovieMode.h"

namespace Movie {

	/* Performs a zoom motion.
	 * The current and target are compared; if a zoom would help move us from A to B,
	 * we move the corners by up to @speed@ pixels.
	 * Returns true if we did something, false if not. */
	bool MotionZoom(const Fractal::Point& size_in, const Fractal::Point& size_target,
			const unsigned width, const unsigned height, const unsigned speed, Fractal::Point& size_out);

	/* Performs a translation motion.
	 * The current and target are compared; if a zoom would help move us from A to B,
	 * we move the corners by up to @speed@ pixels.
	 * Returns true if we did something, false if not. */
	bool MotionTranslate(const Fractal::Point& centre_in, const Fractal::Point& centre_target,
			const Fractal::Point& size,
			const unsigned width, const unsigned height, const unsigned speed, Fractal::Point& centre_out);

}; // namespace Movie

#endif // MOVIEMOTION_H
