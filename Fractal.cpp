/*
    Fractal.cpp: Fractal computation interface and instances
    Copyright (C) 2010 Ross Younger

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

#include "Fractal.h"
#include <assert.h>

using namespace std;

const double _consts::log2 = log(2.0);
_consts consts;

Fractal::Fractal(string name, double xmin, double xmax, double ymin, double ymax) : name(name), xmin(xmin), xmax(xmax), ymin(ymin), ymax(ymax) {}
Fractal::~Fractal() {}

Mandelbrot::Mandelbrot() : Fractal("Mandelbrot", -3.0, 3.0, -3.0, 3.0) {}
Mandelbrot::~Mandelbrot() {}

void Mandelbrot::prepare_pixel(const cdbl coords, fractal_point& out) const
{
	long double o_re = real(coords), o_im = imag(coords);

	// Cardioid check:
	long double t = o_re - 0.25;
	long double im2 = o_im * o_im;
	long double q = t * t + im2;
	if (q*(q + o_re - 0.25) < 0.25*im2)
		goto SHORTCUT;
	// Period-2 bulb check:
	t = o_re + 1.0;
	if (t * t + im2 < 0.0625)
		goto SHORTCUT;

	// The first iteration is easy, 0^2 + origin...
	out.origin = out.point = cdbl(coords);
	out.iter = 1;
	return;

	SHORTCUT:
	out.mark_infinite();
}

void Mandelbrot::plot_pixel(const int maxiter, fractal_point& out) const
{
	// Speed notes:
	// Don't use cdbl in the actual calculation - using straight doubles and
	// doing the complex maths by hand is about 6x faster for me.
	// Also, we know that 0^2 + origin = origin, so skip the first iter.
	int iter;
	long double o_re = real(out.origin), o_im = imag(out.origin);
	long double z_re = real(out.point), z_im = imag(out.point), re2, im2;

#define ITER() do { 							\
		re2 = z_re * z_re;						\
		im2 = z_im * z_im;						\
		z_im = 2 * z_re * z_im + o_im;			\
		z_re = re2 - im2 + o_re;				\
} while (0)

	for (iter=out.iter; iter<maxiter; iter++) {
		ITER();
		if (re2 + im2 > 4.0) {
			// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
			ITER(); ++iter;
			ITER(); ++iter;
			out.iter = iter;
			out.iterf = iter - log(log(re2 + im2)) / consts.log2;
			out.arg = atan2(z_im, z_re);
			return;
		}
	}
	out.iter = iter;
	out.point = cdbl(z_re,z_im);
}
