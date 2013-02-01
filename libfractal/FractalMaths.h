/*
    FractalMaths.h: Maths types for the fractal library
    Copyright (C) 2010-3 Ross Younger

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

#ifndef FRACTALMATHS_H_
#define FRACTALMATHS_H_

#include <complex>

namespace Fractal {

////////////////////////////////////////////////////////////////////////////
// Maths types and traits

typedef long double Value; // short for "fractal value"
typedef std::complex<Value> Point; // "complex fractal point"

#ifdef ENABLE_DOUBLE
#define MAYBE_DO_DOUBLE(_DO) _DO(double,Double,0.00000000000000044408920985006L /* 4.44e-16 */)
#else
#define MAYBE_DO_DOUBLE(_DO)
#endif

#ifdef ENABLE_FLOAT
#define MAYBE_DO_FLOAT(_DO) _DO(float, Float, 0.0000002384185791016)
#else
#define MAYBE_DO_FLOAT(_DO)
#endif

// Master list of all maths types we know about.
// Format: _DO(type name, enum name, minimum pixel size)
#define ALL_MATHS_TYPES(_DO)	\
	MAYBE_DO_FLOAT(_DO) 		\
	MAYBE_DO_DOUBLE(_DO) 		\
	_DO(long double, LongDouble, 0.0000000000000000002168404345L /* 2.16e-19 */)	\

class Maths {
public:
	enum class MathsType {
#define DO_ENUM(type,name,minpix) name,
		ALL_MATHS_TYPES(DO_ENUM)
		MAX,
	};

	static const char* name(MathsType t); // Enum to name conversion
	static Value min_pixel_size(MathsType t); // Minimum pixel size lookup
	static Value smallest_min_pixel_size();
};

}; // namespace Fractal

////////////////////////////////////////////////////////////////////////////

inline Fractal::Point operator/(const Fractal::Point& a, Fractal::Value b) {
	Fractal::Point rv(a);
	rv.real(real(rv)/b);
	rv.imag(imag(rv)/b);
	return rv;
}

#endif /* FRACTALMATHS_H_ */
