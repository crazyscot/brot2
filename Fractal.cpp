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

Fractal::Fractal(string name, double xmin, double xmax, double ymin, double ymax) : name(name), xmin(xmin), xmax(xmax), ymin(ymin), ymax(ymax) {}
Fractal::~Fractal() {}

Mandelbrot::Mandelbrot() : Fractal("Mandelbrot", -3.0, 3.0, -3.0, 3.0) {}
Mandelbrot::~Mandelbrot() {}

void Mandelbrot::plot_pixel(const cdbl origin, const unsigned maxiter, unsigned *iters_out) const
{
	unsigned iter = 0;
	cdbl z = cdbl(0,0);
	while (abs(z) <= 4.0 && iter < maxiter) {
		z = z*z + origin;
		++iter;
	}
	*iters_out = iter;
	// TODO: Further plot params - radius(cabs), dist(carg?).
}
