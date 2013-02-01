/*
    Fractal-internals.h: Definitions internal to libfractal
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
#ifndef FRACTAL_INTERNALS_H_
#define FRACTAL_INTERNALS_H_

#include "FractalMaths.h"
#include "Exception.h"

namespace Fractal {

// Every source file that declares fractals must have a load_ function.
// They are called in Fractal.cpp.
void load_Mandelbrot();
void load_Mandelbar();
void load_Mandeldrop();
void load_Misc();


class Consts {
public:
	static const Value log2;
	static const Value log3;
	static const Value log4;
	static const Value log5;
};

/*
 * Mixin helper class. This enables a single templated fractal definition
 * class to write the iteration code once and have it reused multiple times
 * with different maths types.
 */
template <class IMPL>
class MathsMixin : public IMPL {
public:
	virtual ~MathsMixin() {}

	virtual void prepare_pixel(const Point coords, PointData& out) const {
		IMPL::prepare_pixel_impl(coords,out);
	}

#define DO_PLOT(type,name,minpix) 	\
	case Maths::MathsType::name: 	\
	IMPL::template plot_pixel_impl<type>(maxiter,out); \
	break;

	virtual void plot_pixel(int maxiter, PointData& out, Maths::MathsType type) const {
		switch(type) {
			ALL_MATHS_TYPES(DO_PLOT)
		case Maths::MathsType::MAX:
			THROW(BrotFatalException, "Unhandled maths type!");
		}
	}
};

}; // namespace Fractal

#endif /* FRACTAL_INTERNALS_H_ */
