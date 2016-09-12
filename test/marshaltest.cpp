/*  marshaltest: Unit tests for marshalling code
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

#include "gtest/gtest.h"
#include "marshal.h"

TEST(Marshal, RuntimeChecksPass) {
	EXPECT_TRUE(b2marsh::runtime_type_check());
}

Fractal::Value marshvectors_ld[] = {
	0.0, 1.0, M_PI, INT_MAX, INT_MIN, (long double)9e-318, (long double)1e308
};

// Simple pairwise checks for local type marshalling functions
TEST(Marshal, LongDoublePairwise) {
	for (int i=0; i<sizeof(marshvectors_ld)/sizeof(*marshvectors_ld); i++) {
		Fractal::Value tv = marshvectors_ld[i];

		b2msg::Float wire;
		b2marsh::Value2Wire(tv, &wire);
		Fractal::Value result;
		b2marsh::Wire2Value(wire, result);
		EXPECT_EQ(tv, result);
	}
}

// Confirms that message marshalling is working
TEST(Marshal, MessagePairwise) {
	const Fractal::Point p1(1.2, 3.4);
	const Fractal::Point p2(5.6, 7.8);
	std::string marshalled;
	{
		b2msg::Frame fr;
		b2marsh::Point2Wire(p1, fr.mutable_centre());
		b2marsh::Point2Wire(p2, fr.mutable_size());
		fr.SerializeToString(&marshalled);
	}

	b2msg::Frame fr2;
	fr2.ParseFromString(marshalled);
	Fractal::Point pt;
	b2marsh::Wire2Point(fr2.centre(), pt);
	EXPECT_EQ(pt, p1);
	b2marsh::Wire2Point(fr2.size(), pt);
	EXPECT_EQ(pt, p2);
}
