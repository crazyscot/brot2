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
#include "libbrot2/Exception.h"

using namespace Plot3;

class R2Memory: public ::testing::Test {
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
	const bool _antialias;

	R2Memory(int pixfmt=Render2::pixpack_format::PACKED_RGB_24, bool aa=false) :
			_TestW(37), _TestH(41), _rowstride(0),
			_origin(0.6,0.7), _size(0.001, 0.01),
			_buf(0), _bufz(0), _render(0), _fmt(pixfmt), _antialias(aa) {};
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
		_render = new Render2::MemoryBuffer(_buf, _rowstride, _TestW, _TestH, _antialias, -1, _fmt, _palette);
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

class Render2MemoryFormatP: public ::testing::WithParamInterface<int>, public R2Memory {
protected:
	Render2MemoryFormatP() : R2Memory(GetParam()) {};
};

TEST_P(Render2MemoryFormatP, AllMemTouched) {
	Plot3Chunk chunk(NULL, _fract, _TestW, _TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
	chunk.run();
	_render->process(chunk);
}

///////////////////////////////////////////////////

class R2MemoryMetaTest: public R2Memory {
	/* This checks that we haven't accidentally broken the test mechanism... */
	virtual void FinalExpectation(int fails) {
		// Expect at least 1 failure. Should get the whole buffer failing.
		ASSERT_EQ(_TestW*_TestH*(_rowstride/_TestW), fails) << "The whole buffer should have failed";
	}
};

TEST_F(R2MemoryMetaTest, TestTheTestware)
{
	/* No chunk, nothing to process - we expect the checker to report errors
	 * See the doctored FinalExpectation above. */
}

///////////////////////////////////////////////////

INSTANTIATE_TEST_SUITE_P(AllFormats, Render2MemoryFormatP,
	::testing::Values(Render2::pixpack_format::PACKED_RGB_24, CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGB24));

TEST_F(R2Memory, ChunkOffsetsWork) {
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
#define CHUNK(X,Y,W,H) chunks.push_back(Plot3Chunk(NULL, _fract, W,H, X,Y, _origin, _size, Fractal::Maths::MathsType::LongDouble))
	CHUNK(0,0,                 _TestW-10, _TestH-15);
	CHUNK(_TestW-10,0,         10, _TestH-15);

	CHUNK(0,_TestH-15,         _TestW-10, 15);
	CHUNK(_TestW-10,_TestH-15, 10, 15);
#undef CHUNK

	for (it=chunks.begin(); it!=chunks.end(); it++) {
		(*it).run();
		_render->process(*it);
	}
}

TEST_F(R2Memory, GetWhatWePut) {
	Plot3Chunk chunk(NULL, _fract, _TestW, _TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
	chunk.run();
	_render->process(chunk);

	rgb pixel(1,2,3), pixel2;
	_render->pixel_done(0,0,pixel);
	_render->pixel_get(0,0,pixel2);
	EXPECT_EQ(pixel, pixel2);

	// Now restore it to white so the test harness TearDown doesn't complain.
	rgb white(255,255,255);
	_render->pixel_done(0,0,white);
}
///////////////////////////////////////////////////

class R2MemoryAntiAlias: public R2Memory {
public:
	R2MemoryAntiAlias(int pixfmt=Render2::pixpack_format::PACKED_RGB_24) :
			R2Memory(pixfmt, true) {};
};

TEST_F(R2MemoryAntiAlias, Works) {
	Plot3Chunk chunk(NULL, _fract, 2*_TestW, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
	chunk.run();
	_render->process(chunk);
}

TEST_F(R2MemoryAntiAlias, ChunkOffsetsWork) {
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
#define CHUNK(X,Y,W,H) chunks.push_back(Plot3Chunk(NULL, _fract, 2*(W),2*(H), 2*(X),2*(Y), _origin, _size, Fractal::Maths::MathsType::LongDouble))
	CHUNK(0,0,                 _TestW-10, _TestH-15);
	CHUNK(_TestW-10,0,         10, _TestH-15);

	CHUNK(0,_TestH-15,         _TestW-10, 15);
	CHUNK(_TestW-10,_TestH-15, 10, 15);
#undef CHUNK

	for (it=chunks.begin(); it!=chunks.end(); it++) {
		(*it).run();
		_render->process(*it);
	}
}

TEST_F(R2MemoryAntiAlias, OddWidthAsserts) {
	ASSERT_GT(_TestW, 5);
	{
		Plot3Chunk chunk(NULL, _fract, 5, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk.run();
		EXPECT_THROW(_render->process(chunk), BrotAssert);
	}
	{
		Plot3Chunk chunk2(NULL, _fract, 2*_TestW, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk2.run();
		_render->process(chunk2);
	}
}

TEST_F(R2MemoryAntiAlias, OddHeightAsserts) {
	ASSERT_GT(_TestH, 5);
	{
		Plot3Chunk chunk(NULL, _fract, 2*_TestW, 5, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk.run();
		EXPECT_THROW(_render->process(chunk), BrotAssert);
	}
	{
		Plot3Chunk chunk2(NULL, _fract, 2*_TestW, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk2.run();
		_render->process(chunk2);
	}
}

////////////////////////////////////////////////////////////////////////////////

class Render2PNG: public ::testing::Test {
	/* This checks that Render touches every pixel in the output buffer. */
protected:
	MockFractal _fract;
	MockPalette _palette;
	unsigned _TestW, _TestH;
	Fractal::Point _origin, _size;
	Render2::PNG* _png;

	Render2PNG() :
			_TestW(38), _TestH(42), // H must be a multiple of 2 for the upscaler test to work; W must too to avoid a valgrind failure
			_origin(0.6,0.7), _size(0.001, 0.01), _png(0) {
	}
	virtual void SetUp() {
		_png = new Render2::PNG(_TestW, _TestH, _palette, -1, false);
	}

	virtual ~Render2PNG() { delete _png; }

	virtual void TearDown() {
		std::ostringstream pngout("");
		_png->write(pngout);
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
	Plot3Chunk chunk(NULL, _fract, _TestW, _TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
	chunk.run();
	_png->process(chunk);
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
#define CHUNK(X,Y,W,H) chunks.push_back(Plot3Chunk(NULL, _fract, W,H, X,Y, _origin, _size, Fractal::Maths::MathsType::LongDouble))
	CHUNK(0,0,                 _TestW-10, _TestH-15);
	CHUNK(_TestW-10,0,         10, _TestH-15);

	CHUNK(0,_TestH-15,         _TestW-10, 15);
	CHUNK(_TestW-10,_TestH-15, 10, 15);
#undef CHUNK

	for (it=chunks.begin(); it!=chunks.end(); it++) {
		(*it).run();
		_png->process(*it);
	}
}

TEST_F(Render2PNG, GetWhatWePut) {
	Plot3Chunk chunk(NULL, _fract, _TestW, _TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
	chunk.run();
	_png->process(chunk);

	rgb pixel(1,2,3), pixel2;
	_png->pixel_done(0,0,pixel);
	_png->pixel_get(0,0,pixel2);
	EXPECT_EQ(pixel, pixel2);

	// Now restore it to white so the test harness TearDown doesn't complain.
	rgb white(255,255,255);
	_png->pixel_done(0,0,white);
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

INSTANTIATE_TEST_SUITE_P(TwoThroughFour,
                        AntiAliasBase,
                        ::testing::Values(2,3,4));

// -----------------------------------------------------------------------------

class PNGAntiAlias: public ::testing::Test {
protected:
	MockFractal _fract;
	MockPalette _palette;
	unsigned _TestW, _TestH;
	Fractal::Point _origin, _size;
	Render2::PNG _png;

	PNGAntiAlias() :
			_TestW(37), _TestH(41),
			_origin(0.6,0.7), _size(0.001, 0.01),
			_png(_TestW, _TestH, _palette, -1, true) {};

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

TEST_F(PNGAntiAlias, Works) {
	Plot3Chunk chunk(NULL, _fract, 2*_TestW, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
	chunk.run();
	_png.process(chunk);
}

TEST_F(PNGAntiAlias, ChunkOffsetsWork) {
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
#define CHUNK(X,Y,W,H) chunks.push_back(Plot3Chunk(NULL, _fract, 2*(W),2*(H), 2*(X),2*(Y), _origin, _size, Fractal::Maths::MathsType::LongDouble))
	CHUNK(0,0,                 _TestW-10, _TestH-15);
	CHUNK(_TestW-10,0,         10, _TestH-15);

	CHUNK(0,_TestH-15,         _TestW-10, 15);
	CHUNK(_TestW-10,_TestH-15, 10, 15);
#undef CHUNK

	for (it=chunks.begin(); it!=chunks.end(); it++) {
		(*it).run();
		_png.process(*it);
	}
}

TEST_F(PNGAntiAlias, OddWidthAsserts) {
	ASSERT_GT(_TestW, 5);
	{
		Plot3Chunk chunk(NULL, _fract, 5, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk.run();
		EXPECT_THROW(_png.process(chunk), BrotAssert);
	}
	{
		Plot3Chunk chunk2(NULL, _fract, 2*_TestW, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk2.run();
		_png.process(chunk2);
	}
}

TEST_F(PNGAntiAlias, OddHeightAsserts) {
	ASSERT_GT(_TestH, 5);
	{
		Plot3Chunk chunk(NULL, _fract, 2*_TestW, 5, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk.run();
		EXPECT_THROW(_png.process(chunk), BrotAssert);
	}
	{
		Plot3Chunk chunk2(NULL, _fract, 2*_TestW, 2*_TestH, 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
		chunk2.run();
		_png.process(chunk2);
	}
}

// -----------------------------------------------------------------------------


class Render2PNGUpscaled: public Render2PNG {
	/* This checks that Render touches every pixel in the output buffer. */
protected:
	Render2PNGUpscaled() : Render2PNG() {}
	virtual void SetUp() {
		_png = new Render2::PNG(_TestW, _TestH, _palette, -1, false, true);
	}
};

TEST_F(Render2PNGUpscaled, Works) {
	Plot3Chunk chunk(NULL, _fract, 0.5*(_TestW+1), 0.5*(_TestH+1), 0, 0, _origin, _size, Fractal::Maths::MathsType::LongDouble);
	chunk.run();
	_png->process(chunk);
	EXPECT_EQ(_TestH, _png->png_height());
	EXPECT_EQ(_TestW, _png->png_width() );
}


// -----------------------------------------------------------------------------

struct OverlayTestData {
	rgb under;
	rgba over;
	rgb expect;
	OverlayTestData(
			unsigned char r1, unsigned char g1, unsigned char b1,
			unsigned char r2, unsigned char g2, unsigned char b2, unsigned char a2,
			unsigned char r3, unsigned char g3, unsigned char b3) : under(r1,g1,b1), over (r2,g2,b2,a2), expect(r3,g3,b3) {}
};

OverlayTestData overlay_vectors[] = {
	// Case 1: Alpha=255 overlays entirely
	OverlayTestData( 42,43,44,  81,82,83,255,  81,82,83 ),
	// Case 2: Alpha=127 overlays halfway
	OverlayTestData( 42,43,44,  82,83,84,127,  61,62,63 ),
	// Case 3: Alpha=0 does nothing
	OverlayTestData( 42,43,44,  81,82,83,0,  42,43,44 ),
	// Case 4: Blacks at alpha=127
	OverlayTestData( 42,43,44,  0,0,0,127,   21,21,22 ),
	// Case 5: Whites at alpha=127
	OverlayTestData( 42,43,44,  255,255,255,127,  148,148,149 ),
};

class OverlayKAT : public ::testing::Test {
public:
	void run_vector(OverlayTestData& v) {
		rgb test(v.under);
		test.overlay(v.over);
		EXPECT_EQ(v.expect.r, test.r);
		EXPECT_EQ(v.expect.g, test.g);
		EXPECT_EQ(v.expect.b, test.b);
	}
	void run_vectors() {
		for (unsigned i=0; i < sizeof(overlay_vectors)/sizeof(*overlay_vectors); i++)
			run_vector(overlay_vectors[i]);
	}
};

TEST_F(OverlayKAT, AnswersCorrect) {
	run_vectors();
}

// -----------------------------------------------------------------------------
