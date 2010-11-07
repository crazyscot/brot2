/*
    Plot.cpp: Fractal plotting engine
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

#include <string>
#include <sstream>
#include <iostream>
#include <assert.h>
#include "Fractal.h"
#include "Plot.h"

Plot::Plot(Fractal* f, cdbl centre, cdbl size,
		unsigned maxiter, unsigned width, unsigned height) :
		fract(f), centre(centre), size(size), maxiter(maxiter),
		width(width), height(height), plot_data_(0) { }

Plot::~Plot() {
	if (plot_data_) delete[] plot_data_;
}

string Plot::info_short() {
	char buf[128];
	std::ostringstream rv;
	rv << fract->name
	   << "@(" << real(centre) << ", " << imag(centre) << ")";

	double avgzoom = 2.0 / (real(size)+imag(size));
	unsigned rr = snprintf(buf, sizeof buf, "%g", avgzoom);
	assert (rr < sizeof buf);
	rv << ", zoom=" << buf;
	rv << ", max=" << maxiter;

	//rv <<   "size=(" << scientific << real(size) << "," << imag(size) << ")";
	return rv.str();
}

void Plot::render() {
	if (plot_data_) delete[] plot_data_;
	plot_data_ = new FPoint[width * height];
	unsigned i,j;
	FPoint * out = plot_data_;
	const cdbl _origin = origin();
	//std::cout << "render centre " << centre << "; size " << size << "; origin " << origin << std::endl;

	complex<double> foo = complex<double>(1,2);
	cdbl colstep = cdbl(real(size) / width,0);
	cdbl rowstep = cdbl(0, imag(size) / height);
	//std::cout << "rowstep " << rowstep << "; colstep "<<colstep << std::endl;

	cdbl render_point = _origin;
	// keep running points.
	for (j=0; j<height; j++) {
		for (i=0; i<width; i++) {
			fract->plot_pixel(render_point, maxiter, &out->iter);
			//std::cout << "Plot " << render_point << " i=" << out->iter << std::endl;
			++out;
			render_point += colstep;
		}
		render_point.real(real(_origin));
		render_point += rowstep;
	}
}

/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates */
cdbl Plot::pixel_to_set(int x, int y)
{
	if (x<0) x=0; else if ((unsigned)x>width) x=width;
	if (y<0) y=0; else if ((unsigned)y>height) y=height;

	const double pixwide = real(size) / width,
		  		 pixhigh  = imag(size) / height;
	cdbl delta (x*pixwide, y*pixhigh);
	return origin() + delta;
}
