/*
    Fractal.h: Fractal computation interface and instances
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

#ifndef FRACTAL_H_
#define FRACTAL_H_

#include <string>
#include <complex>

typedef std::complex<long double> cdbl;

class fractal_point {
public:
	unsigned iter;
};

// Base fractal definition. An instance knows all about a fractal _type_
// but nothing about an individual _plot_ of it (meta-instance?)
class Fractal {
public:
	Fractal(std::string name, double xmin, double xmax, double ymin, double ymax);
	virtual ~Fractal();

	std::string name; // Human-readable
	double xmin, xmax, ymin, ymax; // Maximum useful complex area

	// type ?
	// describe any other params.

	virtual void plot_pixel(const cdbl origin, const unsigned maxiter, fractal_point& out) const = 0;

	// Per-image setup? Orbit? Engine?
	// What is expected of PixelPlot: iters? radius? angle?
};

class Mandelbrot : public Fractal {
public:
	Mandelbrot();
	~Mandelbrot();
	virtual void plot_pixel(const cdbl origin, const unsigned maxiter, fractal_point& iters_out) const;
};

#endif /* FRACTAL_H_ */
