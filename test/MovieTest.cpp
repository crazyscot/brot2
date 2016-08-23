/*  MovieTest: Unit tests for Movie mode
    Copyright (C) 2016 Ross Younger

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
#include "MovieMode.h"
#include "MovieMotion.h"
#include "MovieRender.h"
#include "IMovieProgress.h"
#include "libbrot2/Exception.h"
#include "MockFractal.h"
#include "MockPalette.h"
#include "ThreadPool.h"

#define TEST_WIDTH 300
#define TEST_HEIGHT 300
#define TEST_SPEED 1

TEST(Zoom, Identity) {
	Fractal::Point size( 1.0, 1.0 ), size_out;
	EXPECT_FALSE( Movie::MotionZoom(size, size, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, size_out) );
	EXPECT_EQ( size, size_out );
}

TEST(Zoom, DoesSomething) {
	Fractal::Point size( 1.0, 1.0 ), size2( 0.1, 0.1 ), size_out;
	EXPECT_TRUE( Movie::MotionZoom(size, size2, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, size_out) );
	EXPECT_NE( size, size_out );
}

TEST(Zoom, Epsilon) {
	// Very close, truncates the step to hit the target precisely
	Fractal::Point size( 1.0, 1.0 ), size_target( 1.00001, 1.00001 ), size_out;
	EXPECT_TRUE( Movie::MotionZoom(size, size_target, 100, 100, TEST_SPEED, size_out) );
	EXPECT_EQ( size_target, size_out );

	// Not that close, ordinary step
	Fractal::Point size_target_2(1.01, 1.01);
	EXPECT_TRUE( Movie::MotionZoom(size, size_target_2, 100, 100, TEST_SPEED, size_out) );
	EXPECT_NE( size, size_out );
	EXPECT_NE( size_target, size_out );
}

// Runs a case, returns number of frames we would have rendered.
unsigned do_zoom_case(Fractal::Value s1re, Fractal::Value s1im,
				  Fractal::Value s2re, Fractal::Value s2im, const unsigned speed = TEST_SPEED) {
	Fractal::Point size1(s1re, s1im), size2(s2re, s2im), size_working(size1), size_out;
	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " size=" << size_working << std::endl;
		if ( ! Movie::MotionZoom(size_working, size2, TEST_WIDTH, TEST_HEIGHT, speed, size_working) )
			break;
		++i;
	}
	EXPECT_LT(i, 1000000);
	// And check for no overshoot.
	if (s1re < s2re)
		EXPECT_LE(real(size_working), real(size2));
	else
		EXPECT_GE(real(size_working), real(size2));
	if (s1im < s2im)
		EXPECT_LE(imag(size_working), imag(size2));
	else
		EXPECT_GE(imag(size_working), imag(size2));
	return i;
}

TEST(ZoomIn, Terminates) {
	do_zoom_case( 1.0, 1.0, 0.1, 0.1 );
}

TEST(ZoomOut, Terminates) {
	do_zoom_case( 0.1, 0.1, 1.0, 1.0 );
}

TEST(Zoom, NonSquareTerminates) {
	do_zoom_case( 1.0, 0.5, 0.1, 0.2 );
}

// -----------------------------------------------------------------------------

TEST(Translate, Identity) {
	Fractal::Point cent( 1.0, 1.0 ), cent_out, size(0.1, 0.1);
	EXPECT_FALSE( Movie::MotionTranslate(cent, cent, size, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, cent_out) );
	EXPECT_EQ( cent, cent_out );
}

TEST(Translate, DoesSomething) {
	Fractal::Point cent( 1.0, 1.0 ), cent2( 0.1, 0.1 ), cent_out, size(0.5, 0.5);
	EXPECT_TRUE( Movie::MotionTranslate(cent, cent2, size, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, cent_out) );
	EXPECT_NE( cent, cent_out );
}

TEST(Translate, Epsilon) {
	// Very close, precise (truncated) step
	Fractal::Point cent( 1.0, 1.0 ), cent_target( 1.00001, 1.00001 ), cent_out, size(0.5, 0.5);
	EXPECT_TRUE( Movie::MotionTranslate(cent, cent_target, size, 100, 100, TEST_SPEED, cent_out) );
	EXPECT_EQ( cent_target, cent_out );

	// Not that close, ordinary step
	Fractal::Point cent_target_2(1.01, 1.01);
	EXPECT_TRUE( Movie::MotionTranslate(cent, cent_target_2, size, 100, 100, TEST_SPEED, cent_out) );
	EXPECT_NE( cent, cent_out );
	EXPECT_NE( cent_target, cent_out );
}

// Runs a case, returns number of frames we would have rendered.
unsigned do_translate_case(Fractal::Value re1, Fractal::Value im1,
					   Fractal::Value re2, Fractal::Value im2,
					   Fractal::Value reSz=0.1, Fractal::Value imSz=0.1, const unsigned speed = TEST_SPEED) {
	Fractal::Point centre1( re1, im1 ), centre2( re2, im2 ), centre_out, size(reSz, imSz);
	Fractal::Point centre_working(centre1);
	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " centre=" << centre_working << std::endl;
		if ( ! Movie::MotionTranslate(centre_working, centre2, size, TEST_WIDTH, TEST_HEIGHT, speed, centre_working) )
			break;
		++i;
	}
	EXPECT_LT(i, 1000000);
	// And check for no overshoot.
	if (re1 < re2)
		EXPECT_LE(real(centre_working), real(centre2));
	else
		EXPECT_GE(real(centre_working), real(centre2));
	if (im1 < im2)
		EXPECT_LE(imag(centre_working), imag(centre2));
	else
		EXPECT_GE(imag(centre_working), imag(centre2));
	return i;
}

TEST(Translate, Terminates1) { do_translate_case(0.1,0.1, 0.2, 0.2); }
TEST(Translate, Terminates2) { do_translate_case(0.1,0.2, 0.2, 0.1); }
TEST(Translate, Terminates3) { do_translate_case(0.2,0.1, 0.1, 0.2); }
TEST(Translate, Terminates4) { do_translate_case(0.2,0.2, 0.1, 0.1); }
TEST(Translate, Terminates5) { do_translate_case(0.3,0.2, 0.1, 0.1); }
TEST(Translate, Terminates6) { do_translate_case(0.2,0.3, 0.1, 0.1); }
TEST(Translate, Terminates7) { do_translate_case(0.3,0.2, 0.1, 0.0); }
TEST(Translate, Terminates8) { do_translate_case(0.3,0.2, 0.0, 0.1); }

TEST(Translate, NonSquareTerminates) {
	// This is a deliberately contrived example
	do_translate_case( 0.1, 0.1, 0.5, 0.5, 0.1, 0.5 );
}

// -----------------------------------------------------------------------------

TEST(ZoomAndTranslate, IteratesCorrectly) {
	// Start and end co-ords/size
	Fractal::Point centre1(-3.0, 3.0);
	Fractal::Point centre2(-1.03, 0.37);
	Fractal::Point size1(6.0, 6.0);
	Fractal::Point size2(0.375, 0.375);

	// Working registers
	Fractal::Point cc(centre1), ss(size1);

	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " centre=" << cc << ", size=" << ss << std::endl;
		bool active = Movie::MotionZoom(ss, size2, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, ss);
		active |= Movie::MotionTranslate(cc, centre2, ss, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, cc);
		if (!active)
			break;
		++i;
	}
	EXPECT_LT(i, 1000000);
	//std::cout << "DONE " << i << " centre=" << cc << ", size=" << ss << std::endl;
		/* Plumbing into movie panel works fine in move or zoom alone.
		 * Both together doesn't go to the correct out point.
		 * So simulating here, expect correct answer. */
	EXPECT_EQ(cc, centre2);
	EXPECT_EQ(ss, size2);
}

// -----------------------------------------------------------------------------

TEST(Zoom, SpeedBehaves) {
	unsigned s1 = do_zoom_case( 1.0, 1.0, 0.1, 0.1, TEST_SPEED );
	unsigned s2 = do_zoom_case( 1.0, 1.0, 0.1, 0.1, 2*TEST_SPEED );
	EXPECT_LT(s2, s1); // Twice the speed, should give about half the frames.
}

TEST(Translate, SpeedBehaves) {
	unsigned s1 = do_translate_case( 0.1, 0.1, 0.2, 0.2, 0.1, 0.1, TEST_SPEED );
	unsigned s2 = do_translate_case( 0.1, 0.1, 0.2, 0.2, 0.1, 0.1, 2*TEST_SPEED );
	EXPECT_LT(s2, s1); // Twice the speed, should give about half the frames.
}

// -----------------------------------------------------------------------------

class MovieTest : public ::testing::Test {
	protected:
		MockFractal fract;
		MockPalette pal;
		struct Movie::MovieInfo movie;
		ThreadPool threads;

		MovieTest() : threads(0) {}
		void InitialiseMovie(unsigned speed = TEST_SPEED) {
			movie.fractal = &fract;
			movie.palette = &pal;
			movie.width = TEST_WIDTH;
			movie.height = TEST_HEIGHT;
			Movie::KeyFrame f1( 0.1, 0.1, 0.1, 0.1, 0, speed, speed );
			Movie::KeyFrame f2( 0.0, 0.0, 0.1, 0.1, 0, speed, speed );
			movie.points.clear();
			movie.points.push_back(f1);
			movie.points.push_back(f2);
		}
};

// This tests that render actually does something
TEST_F(MovieTest, RenderWorks) {
	InitialiseMovie(TEST_SPEED);
	unsigned count1 = movie.count_frames();
	EXPECT_NE(count1, 0);
}

// This tests that changing the speed changes the number of frames.
TEST_F(MovieTest, SpeedHasAnEffect) {
	InitialiseMovie(TEST_SPEED);
	unsigned count1 = movie.count_frames();
	//std::cout << "Frame count 1: " << count1 << std::endl;
	InitialiseMovie(2*TEST_SPEED);
	unsigned count2 = movie.count_frames();
	//std::cout << "Frame count 2: " << count2 << std::endl;
	/* Doubling the speed for a simple transition without hold, should halve the frame count.
	 * ... except for viewers watching on Valgrind, where floating point inaccuracies are
	 * documented behaviour.  http://valgrind.org/docs/manual/manual-core.html#manual-core.limits
	 *
	 * EXPECT_EQ(count1/2, count2); // fails on valgrind
	 */
	EXPECT_LE(abs(count1/2 - count2), 2);
}

// Found the need for this test the hard way...
TEST(Movie,StructConstructorsBehave) {
	// Construct from explicit Points
	Movie::KeyFrame f1 (Fractal::Point(1.2, 3.4), Fractal::Point(5.6, 7.8), 9, 10, 11);
	EXPECT_EQ(real(f1.centre), 1.2);
	EXPECT_EQ(imag(f1.centre), 3.4);
	EXPECT_EQ(real(f1.size), 5.6);
	EXPECT_EQ(imag(f1.size), 7.8);
	EXPECT_EQ(f1.hold_frames, 9);
	EXPECT_EQ(f1.speed_zoom, 10);
	EXPECT_EQ(f1.speed_translate, 11);

	// Construct from other
	Movie::Frame f2( f1 );
	EXPECT_EQ(f1.centre, f2.centre);
	EXPECT_EQ(f1.size,   f2.size);

	// Construct from explicit Points
	Movie::Frame f3( Fractal::Point(2.3, 4.5), Fractal::Point(6.7, 8.9) );
	EXPECT_EQ(real(f3.centre), 2.3);
	EXPECT_EQ(imag(f3.centre), 4.5);
	EXPECT_EQ(real(f3.size), 6.7);
	EXPECT_EQ(imag(f3.size), 8.9);

	// Construct free-hand from values
	Movie::KeyFrame f4 (1.2, 3.4, 5.6, 7.8, 9, 10, 11);
	EXPECT_EQ(real(f4.centre), 1.2);
	EXPECT_EQ(imag(f4.centre), 3.4);
	EXPECT_EQ(real(f4.size), 5.6);
	EXPECT_EQ(imag(f4.size), 7.8);
	EXPECT_EQ(f4.hold_frames, 9);
	EXPECT_EQ(f4.speed_zoom, 10);
	EXPECT_EQ(f4.speed_translate, 11);

	// Construct free-hand from values
	Movie::Frame f5( 2.3, 4.5, 6.7, 8.9 );
	EXPECT_EQ(real(f5.centre), 2.3);
	EXPECT_EQ(imag(f5.centre), 4.5);
	EXPECT_EQ(real(f5.size), 6.7);
	EXPECT_EQ(imag(f5.size), 8.9);
}

// Hold frames should be counted correctly
TEST_F(MovieTest, HoldFramesWork) {
#define EXPECT_COUNT(_what) do { unsigned _count = movie.count_frames(); EXPECT_EQ(_what, _count); } while(0)
	InitialiseMovie(TEST_SPEED);
	movie.points[0].hold_frames = 0;
	unsigned count1 = movie.count_frames();

	InitialiseMovie(TEST_SPEED);
	movie.points[0].hold_frames = 1;
	EXPECT_COUNT(count1+1);

	InitialiseMovie(TEST_SPEED);
	movie.points[0].hold_frames = 2;
	EXPECT_COUNT(count1+2);

	InitialiseMovie(TEST_SPEED);
	movie.points[0].hold_frames = 1000;
	EXPECT_COUNT(count1+1000);

	InitialiseMovie(TEST_SPEED);
	movie.points[1].hold_frames = 1;
	EXPECT_COUNT(count1+1);

	InitialiseMovie(TEST_SPEED);
	movie.points[0].hold_frames = 1;
	movie.points[1].hold_frames = 2;
	EXPECT_COUNT(count1+3);

	InitialiseMovie(TEST_SPEED);
	movie.points[0].hold_frames = 100;
	movie.points[1].hold_frames = 2;
	EXPECT_COUNT(count1+102);
}

TEST_F(MovieTest, AntiAliasDimensions) {
	Movie::MovieNullProgress prog;
	Movie::NullCompletionHandler comp;
	Movie::NullRenderer ren;
	InitialiseMovie();
	std::shared_ptr<const BrotPrefs::Prefs> prefs = BrotPrefs::Prefs::getMaster();
	std::shared_ptr<ThreadPool> threads(new ThreadPool(1));

	movie.antialias = false;
	Movie::RenderJob job(prog, comp, ren, "", movie, prefs, threads, "");
	EXPECT_EQ(job._rwidth, movie.width);
	EXPECT_EQ(job._rheight, movie.height);

	movie.antialias = true;
	Movie::RenderJob job2(prog, comp, ren, "", movie, prefs, threads, "");
	EXPECT_EQ(job2._rwidth, movie.width*2);
	EXPECT_EQ(job2._rheight, movie.height*2);
}
