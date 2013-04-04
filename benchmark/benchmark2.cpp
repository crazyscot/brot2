/*  benchmark2.cpp: brot2 basic benchmark suite, second try
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
#include "Exception.h"
#include "Benchmarkable.h"
#include <iomanip>

using namespace Fractal;
using namespace std;

class FractalBM : public Benchmarkable
{
public:
	FractalImpl *impl;
	Point coords;
	PointData data;
	Maths::MathsType ty;
	const uint64_t _N;

	FractalBM(Maths::MathsType maths) : impl(0), ty(maths), _N(1<<26) {
		FractalCommon::load_base();
		impl = FractalCommon::registry.get("Mandelbrot");
		if (impl==0) THROW(BrotFatalException, "Cannot find my fractal!");

		/* A point (found by experiment) that's in the set but not
		 * in the special-case cut-off regions */
		coords = { -0.1586536, 1.034804 };
		// coords = { -1.563, -0.0625 }; // Mandeldrop
	}

	virtual uint64_t n_iterations() { return _N; }

protected:
	virtual void run() {
		impl->prepare_pixel(coords, data);
		impl->plot_pixel(_N, data, ty);
		/* Note that plot_pixel converts the point data from the base type
		 * to the templated type - but only once per call. */
	}
};

const string YELLOW("\033[0;33m");
const string DEFAULT("\033[m");


struct stats {
	string title;
	uint64_t time;
	uint64_t n_iters;

	stats(const string& _title, uint64_t _time, uint64_t _n_iters) :
		title(_title), time(_time), n_iters(_n_iters) {}

	void output() const {
		uint64_t sec = time / 1000000, usec = time % 1000000;
		long double mean_us = time * 1000000 / n_iters;

		cout << resetiosflags(ios::showbase)
		     << YELLOW
			 << "Benchmark:               " << title << endl
		     << DEFAULT
		     << setw(3)
		     << "Total time:              " << sec << '.' << setfill('0') << setw(6) << usec << 's' << endl
		     << resetiosflags(ios::showbase)
		     << "Number of iterations:    " << n_iters << endl
		     << "Mean time per iteration: " << mean_us << "us" << endl
		     << endl;
	}
};

void do_bm(Maths::MathsType mt, const string& title) {
	FractalBM bm(mt);

	struct stats st(title, bm.benchmark(), bm.n_iterations());
	st.output();
}

#define DO_BENCHMARK(type,name,minpix)	\
	do_bm(Maths::MathsType::name, #name);

int main(int argc, char**argv) {
	ALL_MATHS_TYPES(DO_BENCHMARK);
	(void) argc;
	(void) argv;
}
