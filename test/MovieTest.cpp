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
	// Very close, no effect
	Fractal::Point size( 1.0, 1.0 ), size_target( 1.00001, 1.00001 ), size_out;
	EXPECT_FALSE( Movie::MotionZoom(size, size_target, 100, 100, TEST_SPEED, size_out) );
	EXPECT_EQ( size, size_out );

	// Not that close, does step
	Fractal::Point size_target_2(1.01, 1.01);
	EXPECT_TRUE( Movie::MotionZoom(size, size_target_2, 100, 100, TEST_SPEED, size_out) );
	EXPECT_NE( size, size_out );
}

TEST(ZoomIn, Terminates) {
	Fractal::Point size1( 1.0, 1.0 ), size2( 0.1, 0.1 ), size_working(size1), size_out;
	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " size=" << size_working << std::endl;
		if ( ! Movie::MotionZoom(size_working, size2, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, size_working) )
			break;
		++i;
	}
	EXPECT_LT(i, 1000000);
	// And check for no overshoot.
	EXPECT_GT(real(size_working), real(size2));
	EXPECT_GT(imag(size_working), imag(size2));
}

TEST(ZoomOut, Terminates) {
	Fractal::Point size1( 0.1, 0.1 ), size2( 1.0, 1.0 ), size_working(size1), size_out;
	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " size=" << size_working << std::endl;
		if ( ! Movie::MotionZoom(size_working, size2, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, size_working) )
			break;
		++i;
	}
	EXPECT_LT(i, 1000000);
}

TEST(Zoom, NonSquareTerminates) {
	// This is a deliberately contrived example which should never happen; higher-level code should
	// fix the aspect ratio.
	Fractal::Point size1( 1.0, 0.5 ), size2( 0.1, 0.2 ), size_working(size1), size_out;
	int i=0;
	while (i<1000000) {
		//std::cout << "Step " << i << " size=" << size_working << std::endl;
		if ( ! Movie::MotionZoom(size_working, size2, TEST_WIDTH, TEST_HEIGHT, TEST_SPEED, size_working) )
			break;
		++i;
	}
	EXPECT_LT(i, 1000000);
}

// -----------------------------------------------------------------------------
