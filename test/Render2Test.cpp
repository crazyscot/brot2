/*  Render2Test: Unit tests for Render2
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

#include <stdlib.h>
#include <array>
#include "gtest/gtest.h"
#include "Fractal.h"
#include "MockFractal.h"
#include "MockPalette.h"
#include "Render2.h"

using namespace Plot3;

class Render2Test: public ::testing::Test {
	/* This checks that Render touches every pixel in the output buffer. */
protected:
	MockFractal _fract;
	unsigned _TestW, _TestH, _rowstride;
	Fractal::Point _origin, _size;
	unsigned char *_buf;
	unsigned _bufz;
	Render2::MemoryBuffer *_render;
	Render2::pixpack_format _fmt;
	MockPalette _palette;

	Render2Test(int pixfmt=Render2::pixpack_format::PACKED_RGB_24) :
			_TestW(37), _TestH(41), _rowstride(0),
			_origin(0.6,0.7), _size(0.001, 0.01),
			_buf(0), _bufz(0), _render(0), _fmt(pixfmt) {};
	virtual void SetUp() {
		switch (_fmt) {
		/* Note: We carefully choose our rowstride so that there are no
		 * untouched bytes in the buffer. */
		case Render2::pixpack_format::PACKED_RGB_24:
			_rowstride = 3 * _TestW;
			break;
		case CAIRO_FORMAT_ARGB32:
		case CAIRO_FORMAT_RGB24:
			_rowstride = 4 * _TestW;
			break;
		default:
			FAIL() << "Unhandled pixfmt";
		}
		_bufz = _rowstride * _TestH;
		_buf = new unsigned char[_bufz];
		std::fill_n(_buf, _bufz, 0);
		_render = new Render2::MemoryBuffer(_buf, _rowstride, _TestW, _TestH, -1, _fmt, _palette);
	}

	virtual void FinalExpectation(int fails) {
		EXPECT_EQ(0, fails) << "non-FF bytes in buffer";
	}

	virtual void TearDown() {
		/* Our MockPalette returns RGB (255,255,255) triplets, so we expect the
		 * buffer to be all-0xff. This may have to be changed if we later add
		 * a pixel format that behaves differently. */
		int fail=0;
		for (unsigned i=0; i<_bufz; i++) {
			if (_buf[i] != 0xff) ++fail;
		}
		FinalExpectation(fail);
		delete _render;
		delete[] _buf;
	}
};

class Render2TestFormatParam: public ::testing::WithParamInterface<int>, public Render2Test {
protected:
	Render2TestFormatParam() : Render2Test(GetParam()) {};
};

TEST_P(Render2TestFormatParam, AllMemTouched) {
	Plot3Chunk chunk(NULL, _fract, _TestW, _TestH, 0, 0, _origin, _size, 10);
	chunk.run();
	_render->process(chunk);
}

class Render2MetaTest: public Render2Test {
	/* This checks that we haven't accidentally broken the test mechanism... */
	virtual void FinalExpectation(int fails) {
		// Expect at least 1 failure. Should get the whole buffer failing.
		ASSERT_EQ(_TestW*_TestH*(_rowstride/_TestW), fails) << "The whole buffer should have failed";
	}
};

TEST_F(Render2MetaTest, TestTheTestware)
{
	/* No chunk, nothing to process - we expect the checker to report errors
	 * See the doctored FinalExpectation above. */
}

INSTANTIATE_TEST_CASE_P(AllFormats, Render2TestFormatParam,
	::testing::Values(Render2::pixpack_format::PACKED_RGB_24, CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGB24));

TEST_F(Render2Test, ChunkOffsetsWork) {
	ASSERT_GT(_TestW, 10);
	ASSERT_GT(_TestH, 15);
	/* +------------------+------------------+
	 * | 0,0 -> W-10,H-15 | W-10,0 -> W,H-15 |
	 * |------------------+------------------|
	 * | 0,H-15 -> W-10,H | W-10,H-15 -> W,H |
	 * +------------------+------------------+
	 */
	std::list<Plot3Chunk> chunks;
	std::list<Plot3Chunk>::iterator it;
#define CHUNK(X,Y,W,H) chunks.push_back(Plot3Chunk(NULL, _fract, W,H, X,Y, _origin, _size, 10))
	CHUNK(0,0,                 _TestW-10, _TestH-15);
	CHUNK(_TestW-10,0,         10, _TestH-15);

	CHUNK(0,_TestH-15,         _TestW-10, 15);
	CHUNK(_TestW-10,_TestH-15, 10, 15);

	for (it=chunks.begin(); it!=chunks.end(); it++) {
		(*it).run();
		_render->process(*it);
	}
}

// -----------------------------------------------------------------------------

class Render2PNG: public ::testing::Test {
	/* This checks that Render touches every pixel in the output buffer. */
protected:
	MockFractal _fract;
	MockPalette _palette;
	unsigned _TestW, _TestH;
	Fractal::Point _origin, _size;
	Render2::PNG _png;

	Render2PNG() :
			_TestW(37), _TestH(41),
			_origin(0.6,0.7), _size(0.001, 0.01),
			_png(_TestW, _TestH, _palette, -1) {};

	virtual void TearDown() {
		std::ostringstream pngout("");
		_png.write(pngout);
		std::istringstream pngin(pngout.str());
		png::image< png::rgb_pixel > png2(pngin);
		// And now apply (broadly) the same pixel-is-touched check as above.
		int fails=0;
		for (unsigned j=0; j<_TestH; j++) {
			for (unsigned i=0; i<_TestW; i++) {
				if (png2[j][i].red != 0xFF) ++fails;
				if (png2[j][i].green != 0xFF) ++fails;
				if (png2[j][i].blue != 0xFF) ++fails;
			}
		}
		EXPECT_EQ(0, fails) << "non-FF rgb bytes in the assembled PNG";
	}
};

TEST_F(Render2PNG, Works) {
	Plot3Chunk chunk(NULL, _fract, _TestW, _TestH, 0, 0, _origin, _size, 10);
	chunk.run();
	_png.process(chunk);
}

TEST_F(Render2PNG, ChunkOffsetsWork) {
	ASSERT_GT(_TestW, 10);
	ASSERT_GT(_TestH, 15);
	/* +------------------+------------------+
	 * | 0,0 -> W-10,H-15 | W-10,0 -> W,H-15 |
	 * |------------------+------------------|
	 * | 0,H-15 -> W-10,H | W-10,H-15 -> W,H |
	 * +------------------+------------------+
	 */
	std::list<Plot3Chunk> chunks;
	std::list<Plot3Chunk>::iterator it;
#define CHUNK(X,Y,W,H) chunks.push_back(Plot3Chunk(NULL, _fract, W,H, X,Y, _origin, _size, 10))
	CHUNK(0,0,                 _TestW-10, _TestH-15);
	CHUNK(_TestW-10,0,         10, _TestH-15);

	CHUNK(0,_TestH-15,         _TestW-10, 15);
	CHUNK(_TestW-10,_TestH-15, 10, 15);

	for (it=chunks.begin(); it!=chunks.end(); it++) {
		(*it).run();
		_png.process(*it);
	}
}

// -----------------------------------------------------------------------------

TEST(AntiAliasBase, Identity) {
	std::vector<rgb> pix;
	for (int i=0; i<100; i++) {
		rgb input(rand()%256,rand()%256,rand()%256);
		pix.push_back(input);
		rgb result = Render2::antialias_pixel(pix);
		EXPECT_EQ(input,result);
		pix.clear();
	}

	// one pixel  identity
	// two pixels, random
	// three, four, ditto
	// special cases: A + black == ?; A + white == ?; r/g/b clip ?
}

class AntiAliasBase : public ::testing::TestWithParam<unsigned> {

};

TEST_P(AntiAliasBase, SameColourUnchanged) {
	std::vector<rgb> pix;
	for (int k=0; k<100; k++) {
		const int n = this->GetParam();
		rgb input(rand()%256,rand()%256,rand()%256);
		for (int i=0; i<n; i++)
			pix.push_back(input);
		rgb result = Render2::antialias_pixel(pix);
		EXPECT_EQ(input,result);
		pix.clear();
	}
}

TEST_P(AntiAliasBase, RandomTests) {
	std::vector<rgb> pix;
	for (int k=0; k<100; k++) {
		const int n = this->GetParam();
		int rr=0,gg=0,bb=0;
		for (int i=0; i<n; i++) {
			rgb input(rand()%256,rand()%256,rand()%256);
			pix.push_back(input);
			rr += input.r;
			gg += input.g;
			bb += input.b;
		}
		rgb myResult(rr/n, gg/n, bb/n);
		rgb result = Render2::antialias_pixel(pix);
		EXPECT_EQ(myResult,result);
		pix.clear();
	}
}

INSTANTIATE_TEST_CASE_P(TwoThroughFour,
                        AntiAliasBase,
                        ::testing::Values(2,3,4));

// -----------------------------------------------------------------------------
