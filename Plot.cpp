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
using namespace std;

Plot::Plot(Fractal* f, cdbl centre, cdbl size,
		unsigned maxiter, unsigned width, unsigned height) :
		fract(f), centre(centre), size(size), maxiter(maxiter),
		width(width), height(height), plot_data_(0) { }

Plot::~Plot() {
	if (plot_data_) delete[] plot_data_;
}

string Plot::info(bool verbose) {
	std::ostringstream rv;
	rv.precision(MAXIMAL_DECIMAL_PRECISION);
	rv << fract->name
	   << "@(" << real(centre) << ", " << imag(centre) << ")";
	rv << ( verbose ? ", maxiter=" : " max=");
	rv << maxiter;

	// Now that we autofix the aspect ratio, our pixels are square.
	double zoom = 1.0/real(size);

	rv.precision(4); // Don't need more than this for the axis length or pixsize.
	if (verbose) {
		char buf[128];
		unsigned rr = snprintf(buf, sizeof buf, "%g", zoom);
		assert (rr < sizeof buf);
		rv << ", zoom=" << buf;
		rv << " / axis length=" << size << " / pixel size=" << real(size)/width;
	} else {
		rv << " axis=" <<size;
	}

	return rv.str();
}

void Plot::prepare() {
	if (plot_data_) delete[] plot_data_;
	plot_data_ = new fractal_point[width * height];
}

void Plot::do_some(const unsigned firstrow, unsigned n_rows) {
	unsigned i,j;
	const cdbl _origin = origin(); // origin of the _whole plot_, not of firstrow
	//std::cout << "render centre " << centre << "; size " << size << "; origin " << origin << std::endl;

	// Sanity check: don't overrun the plot.
	if (firstrow + n_rows > height)
		n_rows = height - firstrow;
	const unsigned endpoint = firstrow + n_rows;

	cdbl colstep = cdbl(real(size) / width,0);
	cdbl rowstep = cdbl(0, imag(size) / height);
	//std::cout << "rowstep " << rowstep << "; colstep "<<colstep << std::endl;

	// TODO: Vertical discontinueties start to crop up around the resolution limit
	// in multi-threaded mode. We ought to detect them and do something appropriate.

	cdbl render_point = _origin;
	render_point.imag(render_point.imag() + firstrow*imag(rowstep));

	unsigned out_index = firstrow * width;
	// keep running points.
	for (j=firstrow; j<endpoint; j++) {
		for (i=0; i<width; i++) {
			fract->plot_pixel(render_point, maxiter, plot_data_[out_index]);
			//std::cout << "Plot " << render_point << " i=" << out->iter << std::endl;
			++out_index;
			render_point += colstep;
		}
		render_point.real(real(_origin));
		render_point += rowstep;
	}
}

void Plot::do_all() {
	prepare();
	do_some(0, height);
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
