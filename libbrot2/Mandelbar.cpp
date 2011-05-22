/*
    Mandelbar.cpp: Implementation of the Mandelbars (Mandelbrot with conjugate)
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
#include "libbrot2.h"
#include <assert.h>
#include <iostream>

using namespace std;

void ensure_Mandelbar(void) {
	// Dummy function so we can forcibly pull in this compile unit
}

// Abstract base class for common unoptimized code
class Mandelbar_Generic : public Fractal {
public:
	Mandelbar_Generic(std::string name_, std::string desc_, fvalue xmin_=-3.0, fvalue xmax_=3.0, fvalue ymin_=-3.0, fvalue ymax_=3.0, unsigned sortorder=20) : Fractal(name_, desc_, xmin_, xmax_, ymin_, ymax_, sortorder) {};
	~Mandelbar_Generic() {};

	virtual void prepare_pixel(const cfpt coords, fractal_point& out) const {
		// The first iteration is easy, 0^k + origin = origin
		out.origin = out.point = cfpt(coords);
		out.iter = 1;
		return;
	};
};

// --------------------------------------------------

class Mandelbar2: public Mandelbar_Generic {
public:
	Mandelbar2(std::string name_, std::string desc_) : Mandelbar_Generic(name_, desc_, -3.0, 3.0, -3.0, 3.0) {};
	~Mandelbar2() {};

	virtual void plot_pixel(const int maxiter, fractal_point& out) const {
		int iter;
		fvalue o_re = real(out.origin), o_im = imag(out.origin),
			   z_re = real(out.point), z_im = imag(out.point), re2, im2;

	#define ITER2() do { 							\
			re2 = z_re * z_re;						\
			im2 = z_im * z_im;						\
			z_im = -2 * z_re * z_im + o_im;			\
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

	};
};

Mandelbar2 mandelbar("Mandelbar (Tricorn)", "z:=(zbar)^2+c");

// --------------------------------------------------

class Mandelbar3: public Mandelbar_Generic {
public:
	Mandelbar3(std::string name_, std::string desc_) : Mandelbar_Generic(name_, desc_, -3.0, 3.0, -3.0, 3.0) {};
	~Mandelbar3() {};

	virtual void plot_pixel(const int maxiter, fractal_point& out) const {
		int iter;
		fvalue o_re = real(out.origin), o_im = imag(out.origin),
			   z_re = real(out.point), z_im = imag(out.point), re2, im2;

#define ITER3() do { 								\
		re2 = z_re * z_re;							\
		im2 = z_im * z_im;							\
		z_re = z_re * re2 - 3*z_re*im2 + o_re; 		\
		z_im = -3 * z_im * re2 + z_im * im2 + o_im; \
} while (0)

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER3();
			if (re2 + im2 > 4.0) {
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

Mandelbar3 mandelbar3("Mandelbar^3", "z:=(zbar)^3+c");

// --------------------------------------------------

class Mandelbar4: public Mandelbar_Generic {
public:
	Mandelbar4(std::string name_, std::string desc_) : Mandelbar_Generic(name_, desc_, -3.0, 3.0, -3.0, 3.0) {};
	~Mandelbar4() {};

	virtual void plot_pixel(const int maxiter, fractal_point& out) const {
		int iter;
		fvalue o_re = real(out.origin), o_im = imag(out.origin),
			   z_re = real(out.point), z_im = imag(out.point), re2, im2;

#define ITER4() do { 								\
		re2 = z_re * z_re;							\
		im2 = z_im * z_im;							\
		z_im = -4 * (re2*z_re*z_im - z_re*im2*z_im) + o_im;	\
		z_re = re2*re2 - 6*re2*im2 + im2*im2 + o_re;		\
} while (0)

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER4();
			if (re2 + im2 > 4.0) {
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

	};
};

Mandelbar4 mandelbar4("Mandelbar^4", "z:=(zbar)^4+c");


// --------------------------------------------------

class Mandelbar5: public Mandelbar_Generic {
public:
	Mandelbar5(std::string name_, std::string desc_) : Mandelbar_Generic(name_, desc_, -3.0, 3.0, -3.0, 3.0) {};
	~Mandelbar5() {};

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
		z_im = -5*re4*z_im + 10*re2*im2*z_im - im4*z_im + o_im;	\
} while (0)

		for (iter=out.iter; iter<maxiter; iter++) {
			ITER5();
			if (re2 + im2 > 4.0) {
				ITER5(); ++iter;
				ITER5(); ++iter;
				out.iter = iter;
				out.iterf = iter - log(log(re2 + im2)) / _consts::log4;
				out.arg = atan2(z_im, z_re);
				out.nomore = true;
				return;
			}
		}
		out.iter = iter;
		out.point = cfpt(z_re,z_im);

	};
};

Mandelbar5 mandelbar5("Mandelbar^5", "z:=(zbar)^5+c");
