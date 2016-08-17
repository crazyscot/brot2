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

#include "MovieMotion.h"

// Returns the sign of a value.
// From http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
#if 0
template <typename T> int sign(T val) {
	return (T(0) < val) - (val < T(0));
}
#endif
int sign(const Fractal::Value val) {
	return (Fractal::Value(0) < val) - (val < Fractal::Value(0));
}

struct signpair {
	int real, imag;

	bool operator==(const struct signpair& other) const {
		return (real == other.real) && (imag == other.imag);
	}
	bool operator!=(const struct signpair& other) const {
		return !(*this == other);
	}
};


// Determines the sign of (a-b) for each dimension independently.
struct signpair calc_signs(const Fractal::Point a, const Fractal::Point b)
{
	Fractal::Point tmp = a - b;
	struct signpair rv;
	rv.real = sign(real(tmp));
	rv.imag = sign(imag(tmp));
	return rv;
}

// Determines the sign of (a-b) for each dimension independently.
Fractal::Point Xcalc_signs(const Fractal::Point a, const Fractal::Point b)
{
	Fractal::Point rv;
	rv.real( real(a) == real(b) ? 0 :
			 real(a) > real(b) ? 1 : -1 );
	rv.imag( imag(a) == imag(b) ? 0 :
			 imag(a) > imag(b) ? 1 : -1 );
	return rv;
}

/* Performs a zoom motion.
 * The current and target are compared; if a zoom would help move us from A to B,
 * we move the corners by up to @speed@ pixels.
 * Returns true if we did something, false if not. */
bool Movie::MotionZoom(const Fractal::Point& size_in, const Fractal::Point& size_target,
		const unsigned width, const unsigned height, const unsigned speed, Fractal::Point& size_out)
{
	// Easy case
	size_out = size_in;
	if ( real(size_in) == real(size_target)  ||  imag(size_in) == imag(size_target) )
		return false;

	int speed_z = speed;
	// If we are zooming IN, size is DECREASING, so the delta is NEGATIVE 
	if (real (size_in) > real(size_target) )
		speed_z = -speed;

	// If we're close enough to target, we're done.
	// But what does "close enough" mean? Good old floating-point comparisons...
	// For our purposes we'll say that if zooming would take us beyond target, we're close enough.
	struct signpair signs_before(calc_signs(size_in, size_target));
	Fractal::Point tmp_out;
	tmp_out.real( real(size_in) * (width + speed_z) / width );
	tmp_out.imag( imag(size_in) * (height + speed_z) / height );
	struct signpair signs_after(calc_signs(tmp_out, size_target));

	if (signs_before != signs_after)
		return false;
	size_out = tmp_out;
	return true;
}
