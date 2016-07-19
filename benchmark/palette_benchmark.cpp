/*  paletteB.cpp: brot2 palette benchmarker
    Copyright (C) 2013-6 Ross Younger

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
#include "palette.h"
#include "Exception.h"
#include "Benchmarkable.h"
#include "Plot3Plot.h"
#include "Render2.h"
#include "cli/CLIDataSink.h"
#include <iomanip>

using namespace Fractal;
using namespace std;
using namespace Plot3;

ThreadPool threads(1);

class PaletteBM : public Benchmarkable
{
public:
	FractalImpl *fract;
	Point centre, size;
	unsigned height, width;
	Plot3Plot *plot;
	CLIDataSink sink;
	Plot3::ChunkDivider::OneChunk divider;

	PaletteBM() : fract(0), height(300), width(300), plot(0), sink(0,true) {
		FractalCommon::load_base();
		fract = FractalCommon::registry.get("Mandelbrot");
		if (fract==0) THROW(BrotFatalException, "Cannot find my fractal!");

		centre = { -0.0946, 1.0105 };
		size = { 0.282, 0.282 };

		plot = new Plot3Plot( threads, &sink, *fract, divider, centre, size, width, height);
		plot->start();

		DiscretePalette::register_base();
		SmoothPalette::register_base();

		plot->wait();
	}

	virtual uint64_t n_iterations() { return height * width; }

	BasePalette *palette;
	void prep(const char* entered_palette) {
		BasePalette *selected_palette = DiscretePalette::all.get(entered_palette);
		if (!selected_palette)
			selected_palette = SmoothPalette::all.get(entered_palette);
		if (!selected_palette)
			THROW(BrotFatalException, "Cannot find my palette!");
		palette = selected_palette;
	}

protected:
	virtual void run() {
		Render2::PNG png(width, height, *palette, -1, false/*AA*/);
		for (auto it : sink._chunks_done)
			png.process(*it);
	}
};

void do_bm(PaletteBM& bm, const char* palette) {
	bm.prep(palette);
	bm.benchmark();
	std::string title("Palette-" + std::string(palette));

	struct stats st(title, bm.benchmark(), bm.n_iterations());
	st.output();
}

#define DO_BENCHMARK(pal)	\
	do_bm(bm, pal);

#define ALL_PALETTES(bm, DO_BENCHMARK) \
	DO_BENCHMARK("Rainbow"); \
	DO_BENCHMARK("Logarithmic rainbow"); \
	DO_BENCHMARK("cos(log)"); \
	DO_BENCHMARK("rjk.mandy"); \
	DO_BENCHMARK("rjk.mandy.blue"); \
	DO_BENCHMARK("fanf.black.fade"); \
	DO_BENCHMARK("fanf.white.fade"); \

int main(int argc, char**argv) {
	PaletteBM bm;
	ALL_PALETTES(bm, DO_BENCHMARK);
	(void) argc;
	(void) argv;
}
