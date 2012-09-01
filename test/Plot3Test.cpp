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

#include "gtest/gtest.h"
#include "libbrot2/Plot3Chunk.h"
#include "libbrot2/IPlot3DataSink.h"
#include "Fractal.h"

class TestPlot3Chunk : public Plot3Chunk {
public:
	bool _calledDone;
	static IPlot3DataSink* _sink;
	static Fractal::FractalImpl* _fract;
	static Fractal::Point _origin;
	static Fractal::Point _size;
#define MAXITER 5

	TestPlot3Chunk (unsigned width, unsigned height) :
		Plot3Chunk(_sink, _fract, _origin, _size, width, height, MAXITER), _calledDone(false) {}
	TestPlot3Chunk (const TestPlot3Chunk& other) : Plot3Chunk(other), _calledDone(false) {}
};

class TestSink : public IPlot3DataSink {
	void pointCheck(const Fractal::PointData& p) {
		/* Sanity checks:
		 * That all the points have suitable data (valgrind checks this for us)
		 * That all the points have been touched (point != origin) (assumes point != 0.0)
		 * That all the points have had their fill of iterations (p.iter == MAXITER) (assumes point iterates infinitely AND is not in the cardioid)
		 * That successive origins differ i.e. are stepping correctly. (p.origin != o_prev)
		 */
		static Fractal::Point o_prev(0,0);
		EXPECT_FALSE(p.nomore);
		EXPECT_NE(p.point, p.origin); // Assumes origin != 0,0.
		EXPECT_EQ(p.iter, MAXITER);
		EXPECT_NE(p.origin, o_prev);
		o_prev = p.origin;
	}
	virtual void chunk_done(Plot3Chunk* job) {
		TestPlot3Chunk* tc = dynamic_cast<TestPlot3Chunk*>(job);
		ASSERT_TRUE(tc!=0);
		tc->_calledDone = true;
		const Fractal::PointData* pp = tc->get_data();
		for (unsigned i=0; i<tc->pixel_count()-1; i++)
			pointCheck(pp[i]);
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

	virtual void SetUp() {
		chunk = 0;
		TestPlot3Chunk::_sink = &sink;
		Fractal::load_Mandelbrot();
		TestPlot3Chunk::_fract = Fractal::FractalCommon::registry.get("Mandelbrot");
		ASSERT_TRUE(TestPlot3Chunk::_fract != 0);
		// _origin and _size are carefully chosen to give some infinitely-iterating points but which are NOT in the cardioid.
		TestPlot3Chunk::_origin = Fractal::Point(-0.13031,0.73594);
		TestPlot3Chunk::_size = Fractal::Point(0.00001,0.00001);
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
