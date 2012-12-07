/*  Plot3Test: Unit tests for Plot3Plot and related classes
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

#define _ISOC99_SOURCE
#include <math.h>

#include <list>
#include <functional>
#include <thread>
#include <chrono>

#include "gtest/gtest.h"
#include "libbrot2/Plot3Chunk.h"
#include "libbrot2/ChunkDivider.h"
#include "libbrot2/IPlot3DataSink.h"
#include "libbrot2/Plot3Plot.h"
#include "libbrot2/Plot3Pass.h"
#include "libbrot2/ThreadPool.h"
#include "MockFractal.h"
#include "MockPrefs.h"
#include "Exception.h"

using namespace std;
using namespace Plot3;
using namespace BrotPrefs;

// Plug our long-double floating point into gtest's floating point comparator:
#define EXPECT_FVAL_EQ(expected,actual) \
	EXPECT_DOUBLE_EQ(expected,actual)
#if 0
// Can't seem to figure out how to do this, so close-enough-for-double
// will have to do for now.
typedef FloatingPoint<Fractal::Value> FractalValue;
#define EXPECT_FVAL_EQ(expected,actual) \
	EXPECT_PRED_FORMAT2( \
			::testing::internal::CmpHelperFloatingPointEQ<Fractal::Value>, \
			expected, actual)
#endif

class TestPlot3Chunk : public Plot3Chunk {
public:
	static IPlot3DataSink* _sink;
	static Fractal::FractalImpl* _fract;
	static Fractal::Point _origin;
	static Fractal::Point _size;
#define MAXITER 5

	TestPlot3Chunk (unsigned width, unsigned height, unsigned offX, unsigned offY) :
		Plot3Chunk(_sink, *_fract, width, height, offX, offY,_origin, _size, MAXITER) {}
	TestPlot3Chunk (const TestPlot3Chunk& other) : Plot3Chunk(other) {}
#undef MAXITER
};

class TestSink : public IPlot3DataSink {
	std::atomic<unsigned> _chunks_count;
	std::atomic<unsigned> _points_count;
	Fractal::Point origin_prev;
	bool* pixels_touched;
	unsigned _height, _width, _double_touched;
	std::mutex _lock;
public:
	Fractal::Value _T,_B,_L,_R; // XXX CONCURRENCY XXX

	TestSink(unsigned width, unsigned height) :
			_chunks_count(0), _points_count(0),
			origin_prev(666,666), pixels_touched(0),
			_height(height), _width(width), _double_touched(0),
			_T(HUGE_VAL), _B(-HUGE_VAL), _L(-HUGE_VAL), _R(HUGE_VAL)
	{
		if (height*width!=0)
			pixels_touched = new bool[height*width]();
	}
	~TestSink() {
		final_check();
		delete[] pixels_touched;
	}

	int chunks_count() const { return _chunks_count; }
	int points_count() const { return _points_count; }

	void reset() {
		reset(_width, _height);
	}
	void reset(unsigned width, unsigned height) {
		/* Needed in some 1x1 corner cases if you expect the same chunk
		 * origin to come back through the same sink again */
		/* Needed if you send more than one set of chunks through
		 * a sink with overlapping pixel addresses. */
		origin_prev.real(HUGE_VAL);
		origin_prev.imag(HUGE_VAL);
		_chunks_count = 0;
		_points_count = 0;
		_height = height;
		_width = width;
		delete[] pixels_touched;
		if (height*width != 0)
			pixels_touched = new bool[height*width]();
		else
			pixels_touched = 0; // Lets us catch an improperly set up run quickly

		_double_touched = 0;
	}

	void pointCheck(Plot3Chunk* job, const Fractal::PointData& p) {
		/* Sanity checks:
		 * That all the points have suitable data (valgrind checks this for us)
		 * That all the points have been touched (point != origin) (assumes point != 0.0)
		 * That all the points have had their fill of iterations (p.iter == MAXITER) (assumes point iterates infinitely AND is not in the cardioid)
		 * That successive origins differ i.e. are stepping correctly. (p.origin != origin_prev)
		 *
		 * We also gather the top/left/bottom/right pixel points seen so the caller can check those out.
		 */
		EXPECT_TRUE(p.nomore);
		EXPECT_NE(p.point, p.origin); // Assumes origin != 0,0.
		EXPECT_EQ(p.iter, job->maxiter());
		EXPECT_NE(p.origin, origin_prev);
		origin_prev = p.origin;
	}
	virtual void chunk_done(Plot3Chunk* job) {
		// This is a critical section when called in parallel.
		std::unique_lock<std::mutex> lock(_lock);

		const Fractal::PointData* pp = job->get_data();
		for (unsigned i=0; i<job->pixel_count(); i++) {
			pointCheck(job, pp[i]);

			Fractal::Point TL = job->_origin;
			Fractal::Point BR = job->_origin+job->_size;
			if (real(BR) < _R) _R = real(BR);
			if (real(TL) > _L) _L = real(TL);
			if (imag(BR) > _B) _B = imag(BR);
			if (imag(TL) < _T) _T = imag(TL);
		}

		_chunks_count += 1;
		_points_count += job->pixel_count();
		/*
		 * When testing the divider, also need to check that the chunks
		 * seam correctly i.e. every output pixel is touched precisely once.
		 */
		int nfailed = false;
		for (unsigned j=0; j<job->_height; j++) {
			for (unsigned i=0; i<job->_width; i++) {
				int addr = ( (job->_offY + j) * _width ) + job->_offX + i;
				if (pixels_touched[addr]) {
					_double_touched++;
					nfailed++;
				}
				pixels_touched[addr] = true;
			}
		}
		EXPECT_EQ(0,nfailed) << " double-touches in job " << std::hex << (void*)(job);
	}

	virtual void pass_complete(string&) {}
	virtual void plot_complete() {}

	virtual void final_check() {
		EXPECT_EQ(0,_double_touched) << _double_touched << " pixel(s) were multiply-touched";
		int untouched = 0;
		for (unsigned j=0; j<_height; j++) {
			for (unsigned i=0; i<_width; i++) {
				if (!pixels_touched[_width*j + i])
					untouched++;
			}
		}
		EXPECT_EQ(0,untouched) << untouched << " pixel(s) were not touched";
	}
};

IPlot3DataSink* TestPlot3Chunk::_sink;
Fractal::FractalImpl* TestPlot3Chunk::_fract;
Fractal::Point TestPlot3Chunk::_origin;
Fractal::Point TestPlot3Chunk::_size;

class ChunkTest : public ::testing::Test {
protected:
	TestPlot3Chunk *chunk;
	TestSink sink;
	MockFractal fract;

	ChunkTest(): chunk(0), sink(0,0) {}

	virtual void SetUp() {
		chunk = 0;
		TestPlot3Chunk::_sink = &sink;
		TestPlot3Chunk::_fract = &fract;
		TestPlot3Chunk::_origin = Fractal::Point(0.1,0.1);
		TestPlot3Chunk::_size = Fractal::Point(0.001,0.001);
	}
	virtual void TearDown() {
		delete chunk;
	}

	void test(int x, int y) {
		sink.reset(x,y);
		chunk = new TestPlot3Chunk(x, y, 0, 0);
		chunk->run();
	}
};
// 1-pixel wide and 1-pixel high are edge cases.
TEST_F(ChunkTest, _1x1) {
	test(1,1);
}
TEST_F(ChunkTest, _1x10) {
	test(1,10);
}
TEST_F(ChunkTest, _10x1) {
	test(10,1);
}
TEST_F(ChunkTest, _10x10) {
	test(10,10);
}

TEST_F(ChunkTest, AssignUnusedChunk) {
	TestPlot3Chunk chunk1(1,1,0,0);
	TestPlot3Chunk *chunk2 = new TestPlot3Chunk(chunk1);
	TestPlot3Chunk chunk3(chunk1);
	sink.reset(1,1);
	chunk1.run();
	sink.reset();
	chunk2->run();
	sink.reset();
	chunk3.run();
	// Sanity checks are provided by the sink.
	delete chunk2;
}
TEST_F(ChunkTest, AssignUsedChunk) {
	test(1,1);
	TestPlot3Chunk chunk4(*chunk);
	TestPlot3Chunk *chunk5 = new TestPlot3Chunk(*chunk);
	sink.reset();
	chunk4.run();
	sink.reset();
	chunk5->run();
	// Sanity checks are provided by the sink.
	delete chunk5;
}

TEST_F(ChunkTest, ReuseChunk) {
	test(1,1);
	sink.reset();
	chunk->run();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_F(ChunkTest,WorksOnThreadPool) {
	TestPlot3Chunk chunk(100,100,0,0);
	sink.reset(100,100);
	ThreadPool tp(1);
	// A std::future for... nothing?! But it seems to work:
	auto res = tp.enqueue<void>([&]{chunk.run();});
	res.get();
}

TEST_F(ChunkTest,ParallelThreadsWork) {
	ThreadPool tp(2);
	sink.reset(100,100);
	list<TestPlot3Chunk> jobs;
	list<future<void> > results;

#define CHUNK(a,b,c,d) jobs.push_back(TestPlot3Chunk(a,b,c,d))
	CHUNK(50,50, 0, 0);
	CHUNK(50,50,50, 0);
	CHUNK(50,50, 0,50);
	CHUNK(50,50,50,50);
#undef CHUNK

	for (auto it = jobs.begin(); it != jobs.end(); it++) {
		results.push_back(tp.enqueue<void>([=]{(*it).run();}));
		// [=] -> pass by current value, which is what we want. Not by ref!!
	}

	for (auto it = results.begin(); it != results.end(); it++) {
		(*it).get();
	}
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class TrickExplodingChunk : public Plot3Chunk {
public:
	TrickExplodingChunk(Plot3Chunk& other) : Plot3Chunk(other) {}
	void plot() {
		throw BrotException("my hovercraft is full of eels");
	}
};

class SlowChunk : public Plot3Chunk {
	int delay;
public:
	SlowChunk(Plot3Chunk& other, int delay_ms) : Plot3Chunk(other), delay(delay_ms) {}
	void plot() {
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		Plot3Chunk::plot();
	}
};

class Plot3PassTest : public ChunkTest {
protected:
	ThreadPool pool;

	Plot3PassTest(): pool(1) {}
};

TEST_F(Plot3PassTest, Basics) {
	chunk = new TestPlot3Chunk(1,1,0,0);
	sink.reset(1,1);

	std::list<Plot3Chunk*> list;
	list.push_back(chunk);
	Plot3Pass pass(pool, list);
	pass.run();
}

TEST_F(Plot3PassTest, AllChunksOnce) {
	sink.reset(10,10);
	TestPlot3Chunk chunk1(7,7,0,0);
	TestPlot3Chunk chunk2(7,3,0,7);
	TestPlot3Chunk chunk3(3,7,7,0);
	TestPlot3Chunk chunk4(3,3,7,7);

	std::list<Plot3Chunk*> list;
	list.push_back(&chunk1);
	list.push_back(&chunk2);
	list.push_back(&chunk3);
	list.push_back(&chunk4);

	Plot3Pass pass(pool, list);
	pass.run();
}

TEST_F(Plot3PassTest, ExceptionsPropagate) {
	sink.reset(1,1);
	TestPlot3Chunk chunk1(1,1,0,0);
	TrickExplodingChunk chunk2(chunk1);

	std::list<Plot3Chunk*> list;
	list.push_back(&chunk1);
	list.push_back(&chunk2);
	// Subtlety: Non-excepting chunks work as expected, the exception doesn't throw them.

	Plot3Pass pass(pool, list);
	EXPECT_THROW(pass.run(), BrotException);
}

TEST_F(Plot3PassTest, BasicConcurrencyCheck) {
	// This provides some assurance that we haven't gone crazy and started
	// returning before all the chunks are cooked.
	sink.reset(2,2);
	TestPlot3Chunk tmpl(2,2,0,0);
	SlowChunk chunk(tmpl,100);

	std::list<Plot3Chunk*> list;
	list.push_back(&chunk);

	Plot3Pass pass(pool, list);
	pass.run();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using namespace ChunkDivider;

class PassTestingSink: public IPlot3DataSink {
	int chunks;
	int passes;
	int completions;

public:
	PassTestingSink() : chunks(0), passes(0), completions(0) {}

	virtual void chunk_done(Plot3Chunk*) {
		++chunks;
	}
	virtual void pass_complete(std::string&) {
		++passes;
	}
	virtual void plot_complete() {
		++completions;
	}

	void expect_chunks(int n) { EXPECT_EQ(n, chunks); }
	void expect_passes(int n) { EXPECT_EQ(n, passes); }
	void expect_completions(int n) { EXPECT_EQ(n, completions); }
};

class Plot3Test: public ::testing::Test {
protected:
	const unsigned _W, _H;
	MockFractal fract;
	ThreadPool pool;
	ChunkDivider::OneChunk divider;
	Plot3::Plot3Plot *p3;
	std::shared_ptr<Prefs> prefs;
	Fractal::Point CENTRE, SIZE;

	Plot3Test() : _W(11), _H(19),
			fract(0), pool(1), p3(0), prefs(new MockPrefs()),
			CENTRE(-0.4,-0.4),
			SIZE(0.01,0.2) {}

	virtual void TearDown() {
		delete p3;
	}

	void setDummyPrefs() {
		p3->set_prefs(prefs);
	}

	void notifies_test(int iters_max, int passes_expected)
	{
		PassTestingSink sink; // This tests the notifications
		fract.set_iters(iters_max);
		p3 = new Plot3Plot(pool, &sink, fract, divider,
				CENTRE, SIZE,
				_W, _H, iters_max+1);
		setDummyPrefs();
		p3->start();
		p3->wait();
		sink.expect_chunks(passes_expected * 1);
		sink.expect_passes(passes_expected);
		sink.expect_completions(1);
	}

#define PixelToSetTestTLO(x, y, z) do { \
		Fractal::Point zz = p3->pixel_to_set_tlo(x,y); \
		EXPECT_EQ(z,zz); } while(0)

#define PixelToSetTestBLO(x, y, z) do { \
		Fractal::Point zz = p3->pixel_to_set_blo(x,y); \
		EXPECT_EQ(z,zz); } while(0)

};

TEST_F(Plot3Test, Basics) {
	TestSink sink(_W,_H); // This tests the points are touched
	p3 = new Plot3Plot(pool, &sink, fract, divider,
			CENTRE, SIZE,
			_W, _H, 10);
	setDummyPrefs();
	p3->start();
	p3->wait();
	EXPECT_EQ(1, sink.chunks_count());
	EXPECT_EQ(_W*_H, sink.points_count());
}

TEST_F(Plot3Test, PassNotifies) {
	// For a given iteration limit, how many passes (starting at 1 iter on the first pass) do we take to get there?
	notifies_test(1,1);
	notifies_test(2,2);
	notifies_test(4,4);
	notifies_test(8,7);
	notifies_test(10,8);
	notifies_test(100,15);
}

TEST_F(Plot3Test, PixelToSet) {
	// Known answer tests. We check the corner points, and overruns clip.
	TestSink sink(_W,_H); // This tests the points are touched
	p3 = new Plot3Plot(pool, &sink, fract, divider,
			CENTRE, SIZE,
			_W, _H, 10);
	setDummyPrefs();
	p3->start();
	p3->wait();

//	Fractal::Value LEFT = imag(CENTRE) - imag(SIZE)/2;
//	Fractal::Value RIGHT = LEFT + imag(SIZE);

	// CAUTION: Valgrind fp limitations come into play here, some comparisons fail despite printing out OK.
	// Those marked '///VG' were affected until I tweaked the calculations.
	Fractal::Point BLO_0_0 = CENTRE - SIZE/2.0;
	Fractal::Point BLO_W_0 = BLO_0_0 + real(SIZE);
	Fractal::Point BLO_0_H = CENTRE + SIZE/2.0 - real(SIZE); // Tweaked for VG
	Fractal::Point BLO_W_H = CENTRE + SIZE/2.0; // Tweaked for VG

#define TLO_0_H BLO_0_0
#define TLO_0_0 BLO_0_H
#define TLO_W_H BLO_W_0
#define TLO_W_0 BLO_W_H

	PixelToSetTestBLO(0,0, BLO_0_0);
	PixelToSetTestBLO(-1,-42, BLO_0_0);
	PixelToSetTestTLO(0,0, TLO_0_0); ///VG
	PixelToSetTestTLO(-1,-42, TLO_0_0); ///VG

	PixelToSetTestBLO(_W,0, BLO_W_0);
	PixelToSetTestBLO(_W+1,-42, BLO_W_0);
	PixelToSetTestTLO(_W,0, TLO_W_0); ///VG
	PixelToSetTestTLO(_W+1,-42, TLO_W_0); ///VG

	PixelToSetTestBLO(0,_H, BLO_0_H); ///VG
	PixelToSetTestBLO(-1,_H+1, BLO_0_H); ///VG
	PixelToSetTestTLO(0,_H, TLO_0_H);
	PixelToSetTestTLO(-1,_H+1, TLO_0_H);

	PixelToSetTestBLO(_W,_H, BLO_W_H); ///VG
	PixelToSetTestBLO(_W+1,_H+1, BLO_W_H); ///VG
	PixelToSetTestTLO(_W,_H, TLO_W_H);
	PixelToSetTestTLO(_W+1,_H+1, TLO_W_H);
}


template<typename T>
class ChunkDividerTest : public ::testing::Test {
	public:
		T divider;
	protected:
		MockFractal fract;
		TestSink sink;
		ThreadPool pool;
		Plot3::Plot3Plot *p3;
		Fractal::Point centre, size;

		ChunkDividerTest() : sink(0,0)/*update later*/, pool(1), p3(0),
			centre(-0.4,-0.3), size(0.01,0.02) {}

		virtual void SetUp() {
		}
		virtual void TearDown() {
			EXPECT_FVAL_EQ(imag(centre)-imag(size)/2.0, sink._T);
			EXPECT_FVAL_EQ(real(centre)-real(size)/2.0, sink._L);
			EXPECT_FVAL_EQ(imag(centre)+imag(size)/2.0, sink._B);
			EXPECT_FVAL_EQ(real(centre)+real(size)/2.0, sink._R);
			delete p3;
		}
		void test(int _x, int _y) {
			this->sink.reset(_x,_y);
			this->p3 = new Plot3::Plot3Plot(pool, &this->sink, this->fract, this->divider,
					this->centre, this->size, _x, _y, 10);
			this->p3->start();
			this->p3->wait();
			EXPECT_EQ(_x*_y, this->sink.points_count());
		}
	virtual ~ChunkDividerTest() {}
};

typedef ::testing::Types<OneChunk, Horizontal10px> ChunkTypes;
TYPED_TEST_CASE(ChunkDividerTest, ChunkTypes);

#define CHUNK_DIVIDER_TEST(xx,yy) \
TYPED_TEST(ChunkDividerTest, Check_##xx##_##yy) { this->test(xx,yy); }

CHUNK_DIVIDER_TEST(101,199);
// edge cases:
CHUNK_DIVIDER_TEST(1,101)
CHUNK_DIVIDER_TEST(199,1)
CHUNK_DIVIDER_TEST(1,1)
// one that trips up the 10px horizontal divider:
CHUNK_DIVIDER_TEST(1,50)
