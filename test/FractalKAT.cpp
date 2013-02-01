/*  FractalKAT.cpp: Fractal known-answer tests
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

#include <gtest/gtest.h>
#include "Fractal.h"
#include "Exception.h"

using namespace Fractal;

struct TestData {
	Point coords;
	int iters;
	TestData(Value x, Value y, int its) : coords(x,y), iters(its) {}
};

#define MAXITERS 255

// Points and expected values determined using one test case, others validated against it
TestData vectors[] = {
		{ 0, 0, MAXITERS },
		{ -1.3096, -0.0036, MAXITERS },
		{ -0.186658359, 0.852167414, 36 },
		{ 1.0, 1.0, 4 },
};


class FractalKAT : public ::testing::TestWithParam<Maths::MathsType> {
public:
	FractalImpl *impl;

	virtual void SetUp() {
		FractalCommon::load_base();
		impl = FractalCommon::registry.get("Mandelbrot");
		if (impl==0) THROW(BrotFatalException,"Cannot find my fractal!");
	}
	virtual void TearDown() {
		FractalCommon::unload_registry();
	}

	void run_vector(TestData& v) {
		PointData data;
		impl->prepare_pixel(v.coords, data);
		impl->plot_pixel(MAXITERS, data, GetParam());
		EXPECT_EQ(data.iter, v.iters);
		if (v.iters != MAXITERS)
			EXPECT_TRUE(data.nomore);
	}

	void run_vectors() {
		if (GetParam() == Maths::MathsType::MAX)
			return;
		for (unsigned i=0; i < sizeof(vectors)/sizeof(*vectors); i++)
			run_vector(vectors[i]);
	}
};

TEST_P(FractalKAT, AnswersCorrect) {
	run_vectors();
}

#define DO_TYPES(type,name,minpix) Maths::MathsType::name,

INSTANTIATE_TEST_CASE_P(AllMathTypes, FractalKAT,
		::testing::Values(
				ALL_MATHS_TYPES(DO_TYPES)
				Maths::MathsType::MAX // dummy to terminate
				));
