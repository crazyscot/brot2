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

class Render2Test: public ::testing::Test {
	/* This checks that Render touches every pixel in the output buffer. */
protected:
	MockFractal _fract;
	unsigned _TestW, _TestH, _rowstride;
	Fractal::Point _origin, _size;
	unsigned char *_buf;
	unsigned _bufz;
	Render2::Generic *_render;
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
		_render = new Render2::Generic(_buf, _rowstride, _TestW/2, _TestH/2, -1, _fmt, _palette);
	}

	virtual void FinalExpectation(int fails) {
		EXPECT_EQ(0, fails) << "non-FF bytes in buffer";
	}

	virtual void TearDown() {
		/* Our MockPalette returns RGB (255,255,255) triplets, so we expect the
		 * buffer to be all-0xff. This may have to be changed if we later add
		 * a pixel format that behaves differently. */
		int sum=0, fail=0;
		for (unsigned i=0; i<_bufz; i++) {
			sum += _buf[i];
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
	Plot3Chunk chunk(NULL, &_fract, _TestW, _TestH, 0, 0, _origin, _size, 10);
	chunk.run();
	_render->process(chunk);
}

INSTANTIATE_TEST_CASE_P(AllFormats, Render2TestFormatParam,
	::testing::Values(Render2::pixpack_format::PACKED_RGB_24, CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGB24));



/*
 * now do an end-to-end with a chunkdivider??
 */

