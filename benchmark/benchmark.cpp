/*  benchmark.cpp: brot2 basic benchmark suite
    Copyright (C) 2013 Ross Younger

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
#include <hayai/hayai.hpp>

using namespace Fractal;

class FractalBM : public Hayai::Fixture
{
public:
	FractalImpl *impl;
	Point coords;
	PointData data;

	virtual void SetUp() {
		FractalCommon::load_base();
		impl = FractalCommon::registry.get("Mandelbrot");
		if (impl==0) throw "Cannot find my fractal!";

		/* A point (found by experiment) that's in the set but not
		 * in the special-case cut-off regions */
		coords = { -0.1586536, 1.034804 };

		impl->prepare_pixel(coords, data);
}

	void doit(value_e ty) {
		impl->plot_pixel(16777216, data, ty);
		/* Note that plot_pixel converts the point data from the base type
		 * to the templated type - but only once per call. */
	}
};

BENCHMARK_F(FractalBM, double, 10, 1)
{
	doit(v_double);
}

BENCHMARK_F(FractalBM, longdouble, 10, 1)
{
	doit(v_long_double);
}
