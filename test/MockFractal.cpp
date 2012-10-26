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

#include "MockFractal.h"
#include <string>

using namespace Fractal;

static const std::string EMPTY = "";

MockFractal::MockFractal(int passes) :
		Fractal::FractalImpl(EMPTY, EMPTY, -1.0, 1.0, -1.0, 1.0, 42), _passes(passes)
{
}

MockFractal::~MockFractal() {}

void MockFractal::prepare_pixel(const Point coords, PointData& out) const
{
	out.origin = coords;
	out.point = coords;
}

void MockFractal::plot_pixel(const int maxiter, PointData& out) const
{
	out.point += out.origin;
	if (!_passes) {
		out.nomore = true;
		out.iter = maxiter;
	} else {
		++out.iter;
		if (out.iter == _passes)
			out.nomore = true;
	}
}
