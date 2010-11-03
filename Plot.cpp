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
	std::ostringstream rv;
	rv << fract->name
	   << ": centre=(" << real(centre) << "," << imag(centre) <<
			"), size=(" << real(size) << "," << imag(size) <<
			"), maxiter=" << maxiter;
	return rv.str();
}

void Plot::render() {
	if (plot_data_) delete[] plot_data_;
	plot_data_ = new FPoint[width * height];
	unsigned i,j;
	FPoint * out = plot_data_;
	// we have centre, need to work that back to origin
	cdbl origin = centre - size/2.0;
	//std::cout << "render centre " << centre << "; size " << size << "; origin " << origin << std::endl;

	complex<double> foo = complex<double>(1,2);
	cdbl colstep = cdbl(real(size) / width,0);
	cdbl rowstep = cdbl(0, imag(size) / height);
	//std::cout << "rowstep " << rowstep << "; colstep "<<colstep << std::endl;

	cdbl render_point = origin;
	// keep running points.
	for (j=0; j<height; j++) {
		for (i=0; i<width; i++) {
			fract->plot_pixel(render_point, maxiter, &out->iter);
			//std::cout << "Plot " << render_point << " i=" << out->iter << std::endl;
			++out;
			render_point += colstep;
		}
		render_point.real(real(origin));
		render_point += rowstep;
	}
}
