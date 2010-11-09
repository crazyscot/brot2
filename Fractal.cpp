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
using namespace std;

Fractal::Fractal(string name, double xmin, double xmax, double ymin, double ymax) : name(name), xmin(xmin), xmax(xmax), ymin(ymin), ymax(ymax) {}
Fractal::~Fractal() {}

Mandelbrot::Mandelbrot() : Fractal("Mandelbrot", -3.0, 3.0, -3.0, 3.0) {}
Mandelbrot::~Mandelbrot() {}

void Mandelbrot::plot_pixel(const cdbl origin, const unsigned maxiter, fractal_point& out) const
{
	// Speed notes:
	// Don't use cdbl in the actual calculation - using straight doubles and
	// doing the complex maths by hand is about 6x faster for me.
	// Also, we know that 0^2 + origin = origin, so skip the first iter.
	unsigned iter;
	long double o_re = real(origin), o_im = imag(origin);
	long double z_re = o_re, z_im = o_im, tmp;

	{
		// Cardioid check:
		long double t = o_re - 0.25;
		long double yy = o_im * o_im;
		long double q = t * t + yy;
		if (q*(q + o_re - 0.25) < 0.25*yy)
			goto SHORTCUT;
		// Period-2 bulb check:
		t = o_re + 1.0;
		if (t * t + yy < 0.0625)
			goto SHORTCUT;
	}

	for (iter=1; iter<maxiter; iter++) {
		if (z_re * z_re + z_im*z_im > 4.0) {
			out.iter = iter;
			return;
		}
		tmp = z_re * z_re - z_im * z_im + o_re;
		z_im = 2 * z_re * z_im + o_im;
		z_re = tmp;
	}
	SHORTCUT:
	out.iter = maxiter;
	// TODO: Further plot params - radius(cabs), dist(carg?).
}
