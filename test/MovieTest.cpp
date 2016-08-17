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
#include "MovieMotion.h"
#include "libbrot2/Exception.h"

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

void do_zoom_case(Fractal::Value s1re, Fractal::Value s1im,
				  Fractal::Value s2re, Fractal::Value s2im) {
	Fractal::Point size1(s1re, s1im), size2(s2re, s2im), size_working(size1), size_out;
	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " size=" << size_working << std::endl;
		if ( ! Movie::MotionZoom(size_working, size2, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, size_working) )
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

void do_translate_case(Fractal::Value re1, Fractal::Value im1,
					   Fractal::Value re2, Fractal::Value im2,
					   Fractal::Value reSz=0.1, Fractal::Value imSz=0.1) {
	Fractal::Point centre1( re1, im1 ), centre2( re2, im2 ), centre_out, size(reSz, imSz);
	Fractal::Point centre_working(centre1);
	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " centre=" << centre_working << std::endl;
		if ( ! Movie::MotionTranslate(centre_working, centre2, size, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, centre_working) )
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
