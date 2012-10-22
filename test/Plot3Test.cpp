/*  Plot3Test: Unit tests for Plot3 and related classes
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

#include "gtest/gtest.h"
#include "libbrot2/Plot3Chunk.h"
#include "libbrot2/ChunkDivider.h"
#include "libbrot2/IPlot3DataSink.h"
#include "libbrot2/Plot3.h"
#include "MockFractal.h"
#include "libjob/SimpleJobEngine.h"

class TestPlot3Chunk : public Plot3Chunk {
public:
	static IPlot3DataSink* _sink;
	static Fractal::FractalImpl* _fract;
	static Fractal::Point _origin;
	static Fractal::Point _size;
#define MAXITER 5

	TestPlot3Chunk (unsigned width, unsigned height) :
		Plot3Chunk(_sink, _fract, width, height, _origin, _size, MAXITER) {}
	TestPlot3Chunk (const TestPlot3Chunk& other) : Plot3Chunk(other) {}
#undef MAXITER
};

class TestSink : public IPlot3DataSink {
	std::atomic<unsigned> _chunks_count;
	std::atomic<unsigned> _points_count;
public:
	Fractal::Value _T,_B,_L,_R; // XXX CONCURRENCY XXX

	TestSink() : _chunks_count(0), _points_count(0),
		_T(HUGE_VAL), _B(-HUGE_VAL), _L(-HUGE_VAL), _R(HUGE_VAL) {}
	int chunks_count() const { return _chunks_count; }
	int points_count() const { return _points_count; }

	void pointCheck(Plot3Chunk* job, const Fractal::PointData& p) {
		/* Sanity checks:
		 * That all the points have suitable data (valgrind checks this for us)
		 * That all the points have been touched (point != origin) (assumes point != 0.0)
		 * That all the points have had their fill of iterations (p.iter == MAXITER) (assumes point iterates infinitely AND is not in the cardioid)
		 * That successive origins differ i.e. are stepping correctly. (p.origin != o_prev)
		 *
		 * We also gather the top/left/bottom/right pixel points seen so the caller can check those out.
		 */
		static Fractal::Point o_prev(0,0);
		EXPECT_TRUE(p.nomore);
		EXPECT_NE(p.point, p.origin); // Assumes origin != 0,0.
		EXPECT_EQ(p.iter, job->maxiter());
		EXPECT_NE(p.origin, o_prev);
		o_prev = p.origin;
	}
	virtual void chunk_done(Plot3Chunk* job) {
		const Fractal::PointData* pp = job->get_data();
		for (unsigned i=0; i<job->pixel_count()-1; i++) {
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
		chunk = new TestPlot3Chunk(x, y);
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
	TestPlot3Chunk chunk1(1,1);
	TestPlot3Chunk *chunk2 = new TestPlot3Chunk(chunk1);
	TestPlot3Chunk chunk3(chunk1);
	chunk1.run();
	chunk2->run();
	chunk3.run();
	// Sanity checks are provided by the sink.
	delete chunk2;
}
TEST_F(ChunkTest, AssignUsedChunk) {
	test(1,1);
	TestPlot3Chunk chunk4(*chunk);
	TestPlot3Chunk *chunk5 = new TestPlot3Chunk(*chunk);
	chunk4.run();
	chunk5->run();
	// Sanity checks are provided by the sink.
	delete chunk5;
}

TEST_F(ChunkTest, ReuseChunk) {
	test(1,1);
	chunk->run();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using namespace ChunkDivider;

class Plot3Test: public ::testing::Test {
protected:
	MockFractal fract;
	TestSink sink;
	Plot3 *p3;

	virtual void SetUp() {
		p3 = new Plot3(&sink, &fract,
				Fractal::Point(-0.4,-0.4),
				Fractal::Point(0.01,0.01),
				101, 199, 10);
	}
	virtual void TearDown() {
		delete p3;
	}
};

TEST_F(Plot3Test, Basics) {
	ChunkDivider::OneChunk divider;
	p3->start(divider);
	p3->wait();
	EXPECT_EQ(1, sink.chunks_count()); // Does not hold in general.
	EXPECT_EQ(101*199, sink.points_count());
}

template<typename T>
class ChunkDividerTest : public ::testing::Test {
	public:
		T divider;
	protected:
		MockFractal fract;
		TestSink sink;
		Plot3 *p3;
		Fractal::Point centre, size;

		ChunkDividerTest() : p3(0),
			centre(-0.4,-0.4), size(0.01,0.01) {}

		virtual void SetUp() {
			p3 = new Plot3(&sink, &fract,
					centre, size,
					101, 199, 10);
		}
		virtual void TearDown() {
			EXPECT_EQ(imag(centre)-imag(size)/2.0, sink._T);
			EXPECT_EQ(real(centre)-real(size)/2.0, sink._L);
			EXPECT_EQ(imag(centre)+imag(size)/2.0, sink._B);
			EXPECT_EQ(real(centre)+real(size)/2.0, sink._R);
			delete p3;
		}
};

typedef ::testing::Types<OneChunk, Horizontal10px> ChunkTypes;
TYPED_TEST_CASE(ChunkDividerTest, ChunkTypes);

TYPED_TEST(ChunkDividerTest, SanityCheck) {
	this->p3->start(this->divider);
	this->p3->wait();
	EXPECT_EQ(101*199, this->sink.points_count());
	// XXX anything else? That the edges seam nicely??
}
