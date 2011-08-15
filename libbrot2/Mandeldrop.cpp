/*
    Mandeldrop.cpp: Mandeldrop alternative
    Copyright (C) 2011 Ross Younger

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

#include "Fractal.h"
#include <assert.h>

using namespace std;
using namespace Fractal;

// Abstract base class for common unoptimized code
class Mandeldrop_Generic : public FractalImpl {
public:
	Mandeldrop_Generic(std::string name_, std::string desc_, Value xmin_=-1.5, Value xmax_=4.5, Value ymin_=-1.0, Value ymax_=1.0) : FractalImpl(name_, desc_, xmin_, xmax_, ymin_, ymax_, 30) {};
	~Mandeldrop_Generic() {};

	virtual void prepare_pixel(const Point coords, PointData& out) const {
		// Prep for the pixel described by 1/z0:
		Value zre = real(coords), zim = imag(coords);
		Point z0_inv = coords / Point(zre*zre - zim*zim, 2*zre*zim);
		out.origin = out.point = Point(z0_inv);
		out.iter = 1;
		return;
	};
};

class Mandeldrop : public Mandeldrop_Generic {
public:
	Mandeldrop(std::string name_, std::string desc_, Value xmin_=-3.0, Value xmax_=3.0, Value ymin_=-3.0, Value ymax_=3.0) : Mandeldrop_Generic(name_, desc_, xmin_, xmax_, ymin_, ymax_) {};
	~Mandeldrop() {};

	static inline void ITER2(Value& o_re, Value& o_im, Value& re2, Value& im2, Value& z_re, Value& z_im) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_im = 2 * z_re * z_im + o_im;
		z_re = re2 - im2 + o_re;
	}
	virtual void plot_pixel(const int maxiter, PointData& out) const {
		// Speed notes:
		// Don't use Point in the actual calculation - using straight doubles and
		// doing the complex maths by hand is about 6x faster for me.
		int iter;
		Value o_re = real(out.origin), o_im = imag(out.origin),
			   z_re = real(out.point), z_im = imag(out.point), re2, im2;

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER2(o_re, o_im, re2, im2, z_re, z_im);
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER2(o_re, o_im, re2, im2, z_re, z_im);
				ITER2(o_re, o_im, re2, im2, z_re, z_im);
				iter+=2;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / Consts::log2;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = Point(z_re,z_im);
	}
};


#define REGISTER(cls, name, desc) do {				\
	cls* cls##impl = new cls(name, desc);			\
	FractalCommon::registry.reg(name, cls##impl);	\
} while(0)

void Fractal::load_Mandeldrop() {
	REGISTER(Mandeldrop, "Mandeldrop", "Inverse Mandelbrot set, z:=z^2+c with z0' := 1/z0");
}

