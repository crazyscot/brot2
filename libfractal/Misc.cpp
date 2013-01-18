/*
    Misc.cpp: Miscellaneous fractals (Burning Ship, ...)
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

using namespace std;
using namespace Fractal;

// Abstract base class for common unoptimized code
class Misc_Generic : public FractalImpl {
public:
	Misc_Generic(std::string name_, std::string desc_, Value xmin_=-3.0, Value xmax_=3.0, Value ymin_=-3.0, Value ymax_=3.0) : FractalImpl(name_, desc_, xmin_, xmax_, ymin_, ymax_, 40) {};
	~Misc_Generic() {};

	virtual void prepare_pixel(const Point coords, PointData& out) const {
		// This fractal is "upside-down" in this co-ordinate system, so invert it.
		// The first iteration is easy, 0^k + origin = origin
		out.point = Point(real(coords), -imag(coords));
		out.origin = out.point;
		out.iter = 1;
		return;
	};
};

#define CONSTRUCT(cls, name, desc) 			  \
	cls(): Misc_Generic(name, desc) {}; \
	~cls() {};

class BurningShip : public Misc_Generic {
public:
	CONSTRUCT(BurningShip, "Burning Ship", "z:=(|Re(z)|+i|Im(z)|)^2+c")

	static inline void ITER2(Value& o_re, Value& o_im, Value& re2, Value& im2, Value& z_re, Value& z_im) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_im = 2.0 * fabsl(z_re * z_im) + o_im;
		z_re = re2 - im2 + o_re;
	}
	virtual void plot_pixel(const int maxiter, PointData& out) const {
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

// --------------------------------------------------------------------

class Celtic : public Misc_Generic {
public:
	CONSTRUCT(Celtic, "Generalised Celtic", "z:=(|Re(z)|+i.Im(z))^2+c")

	static inline void ITER2(Value& o_re, Value& o_im, Value& re2, Value& im2, Value& z_re, Value& z_im) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_im = 2.0 * z_re * z_im + o_im;
		z_re = fabsl(re2 - im2) + o_re;
	}
	virtual void plot_pixel(const int maxiter, PointData& out) const {
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

// --------------------------------------------------------------------

class Variant : public Misc_Generic {
public:
	CONSTRUCT(Variant, "The Variant", "z:=z^2+c with Re(z):=|Re(z)| on odd iterations")

	static inline void ITER2(Value& o_re, Value& o_im, Value& re2, Value& im2, Value& z_re, Value& z_im, const int iter) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_im = 2.0 * z_re * z_im + o_im;
		if (iter%2)
			z_re = fabsl(re2-im2) + o_re;
		else
			z_re = re2 - im2 + o_re;
	}
	virtual void plot_pixel(const int maxiter, PointData& out) const {
		int iter;
		Value o_re = real(out.origin), o_im = imag(out.origin),
			   z_re = real(out.point), z_im = imag(out.point), re2, im2;

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER2(o_re, o_im, re2, im2, z_re, z_im,iter);
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER2(o_re, o_im, re2, im2, z_re, z_im,++iter);
				ITER2(o_re, o_im, re2, im2, z_re, z_im,++iter);
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


// --------------------------------------------------------------------

class BirdOfPrey : public Misc_Generic {
public:
	CONSTRUCT(BirdOfPrey, "Bird Of Prey", "z:=(Re(z)+i|Im(z)|)^2+c")

	static inline void ITER2(Value& o_re, Value& o_im, Value& re2, Value& im2, Value& z_re, Value& z_im) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_im = 2.0 * z_re * fabsl(z_im) + o_im;
		z_re = re2 - im2 + o_re;
	}
	virtual void plot_pixel(const int maxiter, PointData& out) const {
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

// --------------------------------------------------------------------

#define REGISTER(cls) do { 										\
	cls* cls##impl = new cls();           						\
	FractalCommon::registry.reg(cls##impl->name, cls##impl);	\
} while(0)

void Fractal::load_Misc() {
	REGISTER(BurningShip);
	REGISTER(Celtic);
	REGISTER(Variant);
	REGISTER(BirdOfPrey);
}

