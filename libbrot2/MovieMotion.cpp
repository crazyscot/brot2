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

	int speed_z = speed * width / 300;
	// If we are zooming IN, size is DECREASING, so the delta is NEGATIVE 
	if (real (size_in) > real(size_target) )
		speed_z = -speed_z;
	int speed_x = speed_z;
	double speed_y = (double) speed_x * height / width;

	// If we're close enough to target, we're done.
	// But what does "close enough" mean? Good old floating-point comparisons...
	// For our purposes we'll say that if zooming would take us beyond target, we're close enough.
	struct signpair signs_before(calc_signs(size_in, size_target));
	Fractal::Point tmp_out;
	tmp_out.real( real(size_in) * (width + speed_x) / width );
	tmp_out.imag( imag(size_in) * (height + speed_y) / height );
	struct signpair signs_after(calc_signs(tmp_out, size_target));

	// Action on "close enough": set output dimension precisely from input; next time the Easy Case check will return false.
	if (signs_before.real != signs_after.real)
		tmp_out.real( real(size_target) );
	if (signs_before.imag != signs_after.imag)
		tmp_out.imag( imag(size_target) );
	size_out = tmp_out;
	return true;
}

/* Performs a translation motion.
 * The current and target are compared; if a zoom would help move us from A to B,
 * we move the corners by up to @speed@ pixels.
 * Returns true if we did something, false if not. */
bool Movie::MotionTranslate(const Fractal::Point& centre_in, const Fractal::Point& centre_target,
		const Fractal::Point& size,
		const unsigned width, const unsigned height, const unsigned speed, Fractal::Point& centre_out)
{

	centre_out = centre_in;
	// Easy case. Note BOTH must match.
	if ( real(centre_in) == real(centre_target)  &&  imag(centre_in) == imag(centre_target) )
		return false;

	// Like with zooming, if moving would take us beyond the target, we're close enough.
	struct signpair signs_before(calc_signs(centre_target, centre_in));
	Fractal::Point tmp_out;

	unsigned speed_x = speed * width / 300;
	double speed_y = (double) speed_x * height / width;
	Fractal::Point pixel_size ( real(size) / width, imag(size) / height );
	Fractal::Point delta ( real(pixel_size) * speed_x, imag(pixel_size) * speed_y );

	if (signs_before.real < 0)
		delta.real(real(delta) * -1.0);
	if (signs_before.imag < 0)
		delta.imag(imag(delta) * -1.0);
	tmp_out = centre_in + delta;

	struct signpair signs_after(calc_signs(centre_target, tmp_out));

	// Action on "close enough": set output dimension precisely from input; next time the Easy Case check will return false.
	if (signs_before.real != signs_after.real)
		tmp_out.real( real(centre_target) );
	if (signs_before.imag != signs_after.imag)
		tmp_out.imag( imag(centre_target) );
	centre_out = tmp_out;
	return true;
}
