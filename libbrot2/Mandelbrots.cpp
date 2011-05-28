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
#include <assert.h>

using namespace std;

void load_Mandelbrot() { }

// Abstract base class for common unoptimized code
class Mandelbrot_Generic : public Fractal {
public:
	Mandelbrot_Generic(std::string name_, std::string desc_, fvalue xmin_=-3.0, fvalue xmax_=3.0, fvalue ymin_=-3.0, fvalue ymax_=3.0) : Fractal(name_, desc_, xmin_, xmax_, ymin_, ymax_, 10) {};
	~Mandelbrot_Generic() {};

	virtual void prepare_pixel(const cfpt coords, fractal_point& out) const {
		// The first iteration is easy, 0^k + origin = origin
		out.origin = out.point = cfpt(coords);
		out.iter = 1;
		return;
	};
};

class Mandelbrot : public Mandelbrot_Generic {
public:
	Mandelbrot(std::string name_, std::string desc_, fvalue xmin_=-3.0, fvalue xmax_=3.0, fvalue ymin_=-3.0, fvalue ymax_=3.0) : Mandelbrot_Generic(name_, desc_, xmin_, xmax_, ymin_, ymax_) {};
	~Mandelbrot() {};

	virtual void prepare_pixel(const cfpt coords, fractal_point& out) const {
		fvalue o_re = real(coords), o_im = imag(coords);

		// Cardioid check:
		fvalue t = o_re - 0.25;
		fvalue im2 = o_im * o_im;
		fvalue q = t * t + im2;
		if (q*(q + o_re - 0.25) < 0.25*im2)
			goto SHORTCUT;
		// Period-2 bulb check:
		t = o_re + 1.0;
		if (t * t + im2 < 0.0625)
			goto SHORTCUT;

		// The first iteration is easy, 0^2 + origin...
		out.origin = out.point = cfpt(coords);
		out.iter = 1;
		return;

		SHORTCUT:
		out.mark_infinite();
	}
	virtual void plot_pixel(const int maxiter, fractal_point& out) const {
		// Speed notes:
		// Don't use cfpt in the actual calculation - using straight doubles and
		// doing the complex maths by hand is about 6x faster for me.
		int iter;
		fvalue o_re = real(out.origin), o_im = imag(out.origin),
			   z_re = real(out.point), z_im = imag(out.point), re2, im2;

	#define ITER2() do { 							\
			re2 = z_re * z_re;						\
			im2 = z_im * z_im;						\
			z_im = 2 * z_re * z_im + o_im;			\
			z_re = re2 - im2 + o_re;				\
	} while (0)

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER2();
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER2(); ++iter;
				ITER2(); ++iter;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / _consts::log2;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = cfpt(z_re,z_im);
	}
};

Mandelbrot mandelbrot("Mandelbrot", "The original Mandelbrot set, z:=z^2+c");

// --------------------------------------------------------------------

class Mandel3 : public Mandelbrot_Generic {
public:
	Mandel3(string name_, string desc_) : Mandelbrot_Generic(name_, desc_) {};
	~Mandel3() {};

	virtual void plot_pixel(const int maxiter, fractal_point& out) const {
		int iter;
		fvalue o_re = real(out.origin), o_im = imag(out.origin),
				z_re = real(out.point), z_im = imag(out.point), re2, im2;

#define ITER3() do { 								\
		re2 = z_re * z_re;							\
		im2 = z_im * z_im;							\
		z_re = z_re * re2 - 3*z_re*im2 + o_re; 		\
		z_im = 3 * z_im * re2 - z_im * im2 + o_im; 	\
} while (0)

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER3();
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER3(); ++iter;
				ITER3(); ++iter;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / _consts::log3;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = cfpt(z_re,z_im);
	};
};

Mandel3 mandel3("Mandelbrot^3", "z:=z^3+c");

// --------------------------------------------------------------------

class Mandel4 : public Mandelbrot_Generic {
public:
	Mandel4(string name_, string desc_) : Mandelbrot_Generic(name_, desc_) {};
	~Mandel4() {};

	virtual void plot_pixel(const int maxiter, fractal_point& out) const {
		int iter;
		fvalue o_re = real(out.origin), o_im = imag(out.origin),
				z_re = real(out.point), z_im = imag(out.point), re2, im2;

#define ITER4() do { 								\
		re2 = z_re * z_re;							\
		im2 = z_im * z_im;							\
		z_im = 4 * (re2*z_re*z_im - z_re*im2*z_im) + o_im;	\
		z_re = re2*re2 - 6*re2*im2 + im2*im2 + o_re;		\
} while (0)

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER4();
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER4(); ++iter;
				ITER4(); ++iter;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / _consts::log4;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = cfpt(z_re,z_im);
	}
};

Mandel4 mandel4("Mandelbrot^4", "z:=z^4+c");

// --------------------------------------------------------------------

class Mandel5 : public Mandelbrot_Generic {
public:
	Mandel5(string name_, string desc_) : Mandelbrot_Generic(name_, desc_) {};
	~Mandel5() {};

	virtual void plot_pixel(const int maxiter, fractal_point& out) const {
		int iter;
		fvalue o_re = real(out.origin), o_im = imag(out.origin),
				z_re = real(out.point), z_im = imag(out.point), re2, im2, re4, im4;

#define ITER5() do { 								\
		re2 = z_re * z_re;							\
		im2 = z_im * z_im;							\
		re4 = re2 * re2;							\
		im4 = im2 * im2;							\
		z_re = re4*z_re - 10*z_re*re2*im2 + 5*z_re*im4 + o_re;	\
		z_im = 5*re4*z_im - 10*re2*im2*z_im + im4*z_im + o_im;	\
} while (0)

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER5();
			if (re2 + im2 > 4.0) {
				// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
				ITER5(); ++iter;
				ITER5(); ++iter;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / _consts::log5;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = cfpt(z_re,z_im);
	};
};

Mandel5 Mandel5("Mandelbrot^5", "z:=z^5+c");
