/*
    Mandelbrots.cpp: Mandelbrot set and immediate derivatives
    Copyright (C) 2010 Ross Younger

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
class Mandelbrot_Generic : public FractalImpl {
public:
	Mandelbrot_Generic(string name, string desc,
			Value xmin_=-3.0, Value xmax_=3.0, Value ymin_=-3.0, Value ymax_=3.0) :
				FractalImpl(name, desc, xmin_, xmax_, ymin_, ymax_, 10) {}
	~Mandelbrot_Generic() {}

protected:
	static void prepare_pixel_impl(const Point coords, PointData& out) {
		// The first iteration is easy, 0^k + origin = origin
		out.origin = out.point = Point(coords);
		out.iter = 1;
		return;
	};
};

#define DECLARE(cls) \
	class cls : public Mandelbrot_Generic

#define CONSTRUCT(cls, name, desc) 			  \
	cls(): Mandelbrot_Generic(name, desc) {}; \
	~cls() {};

DECLARE(Mandelbrot)
{
public:
	CONSTRUCT(Mandelbrot, "Mandelbrot", "The original Mandelbrot set, z:=z^2+c")

	static void prepare_pixel_impl(const Point coords, PointData& out) {
		Value o_re = real(coords), o_im = imag(coords);

		// Cardioid check:
		Value t = o_re - 0.25;
		Value im2 = o_im * o_im;
		Value q = t * t + im2;
		if (q*(q + o_re - 0.25) < 0.25*im2)
			goto SHORTCUT;
		// Period-2 bulb check:
		t = o_re + 1.0;
		if (t * t + im2 < 0.0625)
			goto SHORTCUT;

		// The first iteration is easy, 0^2 + origin...
		out.origin = out.point = Point(coords);
		out.iter = 1;
		return;

		SHORTCUT:
		out.mark_infinite();
	}

	template <typename MATH_T>
	static inline void ITER2(MATH_T& o_re, MATH_T& o_im, MATH_T& re2, MATH_T& im2, MATH_T& z_re, MATH_T& z_im) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_im = 2 * z_re * z_im + o_im;
		z_re = re2 - im2 + o_re;
	}

	template <typename MATH_T>
	static void plot_pixel_impl(const int maxiter, PointData& out) {
		int iter;
		MATH_T	o_re = real(out.origin), o_im = imag(out.origin),
				z_re = real(out.point), z_im = imag(out.point), re2, im2;

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER2(o_re, o_im, re2, im2, z_re, z_im);
			if (re2 + im2 > MATH_T(4.0)) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER2(o_re, o_im, re2, im2, z_re, z_im);
				ITER2(o_re, o_im, re2, im2, z_re, z_im);
				iter+=2;
				out.iter = iter;
				out.iterf = iter - logl(logl(re2 + im2)) / Consts::log2;
				out.arg = atan2l(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = Point(z_re,z_im);
	}
};

// --------------------------------------------------------------------

DECLARE(Mandel3) {
public:
	CONSTRUCT(Mandel3, "Mandelbrot^3", "z:=z^3+c")

	template <typename MATH_T>
	static inline void ITER3(MATH_T& o_re, MATH_T& o_im, MATH_T& re2, MATH_T& im2, MATH_T& z_re, MATH_T& z_im) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_re = z_re * re2 - 3*z_re*im2 + o_re;
		z_im = 3 * z_im * re2 - z_im * im2 + o_im;
	}

	template <typename MATH_T>
	static void plot_pixel_impl(const int maxiter, PointData& out) {
		int iter;
		MATH_T	o_re = real(out.origin), o_im = imag(out.origin),
				z_re = real(out.point), z_im = imag(out.point), re2, im2;
		for (iter=out.iter; iter<maxiter; iter++) {
			ITER3(o_re, o_im, re2, im2, z_re, z_im);
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER3(o_re, o_im, re2, im2, z_re, z_im);
				ITER3(o_re, o_im, re2, im2, z_re, z_im);
				iter+=2;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / Consts::log3;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = Point(z_re,z_im);
	};
};

// --------------------------------------------------------------------

DECLARE(Mandel4) {
public:
	CONSTRUCT(Mandel4, "Mandelbrot^4", "z:=z^4+c")

	template <typename MATH_T>
	static inline void ITER4(MATH_T& o_re, MATH_T& o_im, MATH_T& re2, MATH_T& im2, MATH_T& z_re, MATH_T& z_im) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		z_im = 4 * (re2*z_re*z_im - z_re*im2*z_im) + o_im;
		z_re = re2*re2 - 6*re2*im2 + im2*im2 + o_re;
	}
	template <typename MATH_T>
	static void plot_pixel_impl(const int maxiter, PointData& out) {
		int iter;
		MATH_T o_re = real(out.origin), o_im = imag(out.origin),
				z_re = real(out.point), z_im = imag(out.point), re2, im2;

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER4(o_re, o_im, re2, im2, z_re, z_im);
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER4(o_re, o_im, re2, im2, z_re, z_im);
				ITER4(o_re, o_im, re2, im2, z_re, z_im);
				iter+=2;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / Consts::log4;
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

DECLARE(Mandel5) {
public:
	CONSTRUCT(Mandel5, "Mandelbrot^5", "z:=z^5+c")

	template <typename MATH_T>
	static inline void ITER5(MATH_T& o_re, MATH_T& o_im, MATH_T& re2, MATH_T& im2, MATH_T& z_re, MATH_T& z_im, MATH_T& re4, MATH_T& im4) {
		re2 = z_re * z_re;
		im2 = z_im * z_im;
		re4 = re2 * re2;
		im4 = im2 * im2;
		z_re = re4*z_re - 10*z_re*re2*im2 + 5*z_re*im4 + o_re;
		z_im = 5*re4*z_im - 10*re2*im2*z_im + im4*z_im + o_im;
	}

	template <typename MATH_T>
	static void plot_pixel_impl(const int maxiter, PointData& out) {
		int iter;
		MATH_T o_re = real(out.origin), o_im = imag(out.origin),
				z_re = real(out.point), z_im = imag(out.point), re2, im2, re4, im4;

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER5(o_re, o_im, re2, im2, z_re, z_im, re4, im4);
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER5(o_re, o_im, re2, im2, z_re, z_im, re4, im4);
				ITER5(o_re, o_im, re2, im2, z_re, z_im, re4, im4);
				iter+=2;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / Consts::log5;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = Point(z_re,z_im);
	};
};

#define REGISTER(cls) do { 		\
	auto impl = new MathsMixin<cls>(); \
	(void)impl;			\
} while(0)

void Fractal::load_Mandelbrot() {
	REGISTER(Mandelbrot);
	REGISTER(Mandel3);
	REGISTER(Mandel4);
	REGISTER(Mandel5);
}
