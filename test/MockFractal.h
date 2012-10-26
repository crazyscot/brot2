/*
    MockFractal.h: For unit testing
    Copyright (C) 2012 Ross Younger

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

#ifndef MOCKFRACTAL_H_
#define MOCKFRACTAL_H_

#include "Fractal.h"

class MockFractal: public Fractal::FractalImpl {
	int _iters;
public:
	/**
	 * If _iters==0, we set 'nomore' after a single pass, with iter := maxiter.
	 * Otherwise, we set 'nomore' after that many iterations.
	 */
	MockFractal(int passes=0);
	virtual ~MockFractal();

	void set_iters (int iters) { _iters = iters; }

	virtual void prepare_pixel(const Fractal::Point coords, Fractal::PointData& out) const;
	virtual void plot_pixel(const int maxiter, Fractal::PointData& out) const;
};

#endif /* MOCKFRACTAL_H_ */
