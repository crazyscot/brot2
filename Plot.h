/*
    Plot.h: Fractal plotting engine
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

#ifndef PLOT_H_
#define PLOT_H_

#include <string>
#include <complex>

#include "Fractal.h"

// A plot is an abstract rendering of a Fractal.
// It has fractal centre co-ordinates and size, as well as a plotting size in pixels.
// The rendered data is abstract (iteration count etc).
class Plot {
public:
	Plot(Fractal* f, cdbl centre, cdbl size, unsigned maxiter, unsigned width, unsigned height);
	virtual ~Plot();

	const Fractal* fract;
	const cdbl centre, size;
	const unsigned maxiter;
	const unsigned width, height; // plot size in pixels

	const cdbl origin() { return centre - size/(long double)2.0; }

	// Returns a human-readable summary of this plot for the status bar.
	virtual std::string info_short();

	// Returns the plot. The data is packed one row at a time, top row first; left-to-right within each row.
	const fractal_point * get_plot_data() {
		return plot_data_;
	}

	/* Initialises a plot, allocates memory. NOT THREAD SAFE. */
	void prepare();

	/* Plots one or more rows. Safe to call from multiple threads concurrently. */
	void do_some(unsigned firstrow, unsigned n_rows);

	/* Convenience helper to prepare() and do_some(0,height). */
	void do_all();

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render */
	cdbl pixel_to_set(int x, int y);

protected:
	fractal_point *plot_data_;
};

#endif /* PLOT_H_ */
