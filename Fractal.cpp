/*
    Fractal.cpp: Fractal computation interface and instances
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

std::map<std::string,Fractal*> Fractal::registry;

const fvalue _consts::log2 = log(2.0);
const fvalue _consts::log3 = log(3.0);
_consts consts;

class Mandelbrot : public Fractal {
public:
	Mandelbrot(std::string name_, fvalue xmin_=-3.0, fvalue xmax_=3.0, fvalue ymin_=-3.0, fvalue ymax_=3.0) : Fractal(name_, xmin_, xmax_, ymin_, ymax_) {};
	~Mandelbrot() {};

	virtual void prepare_pixel(const cfpt coords, fractal_point& out) const;
	virtual void plot_pixel(const int maxiter, fractal_point& iters_out) const;
};

void Mandelbrot::prepare_pixel(const cfpt coords, fractal_point& out) const
{
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

void Mandelbrot::plot_pixel(const int maxiter, fractal_point& out) const
{
	// Speed notes:
	// Don't use cfpt in the actual calculation - using straight doubles and
	// doing the complex maths by hand is about 6x faster for me.
	int iter;
	fvalue o_re = real(out.origin), o_im = imag(out.origin),
		   z_re = real(out.point), z_im = imag(out.point), re2, im2;

#define ITER() do { 							\
		re2 = z_re * z_re;						\
		im2 = z_im * z_im;						\
		z_im = 2 * z_re * z_im + o_im;			\
		z_re = re2 - im2 + o_re;				\
} while (0)

	for (iter=out.iter; iter<maxiter; iter++) {
		ITER();
		if (re2 + im2 > 4.0) {
			// Fractional escape count: See http://linas.org/art-gallery/escape/escape.html
			ITER(); ++iter;
			ITER(); ++iter;
			out.iter = iter;
			out.iterf = iter - log(log(re2 + im2)) / consts.log2;
			out.arg = atan2(z_im, z_re);
			out.nomore = true;
			return;
		}
	}
	out.iter = iter;
	out.point = cfpt(z_re,z_im);
}

Mandelbrot mandelbrot("Mandelbrot");
#undef ITER

// --------------------------------------------------------------------

class Mandel3 : public Mandelbrot {
public:
	Mandel3(std::string name_, fvalue xmin_=-3.0, fvalue xmax_=3.0, fvalue ymin_=-3.0, fvalue ymax_=3.0) : Mandelbrot(name_, xmin_, xmax_, ymin_, ymax_) {};
	~Mandel3() {};

	virtual void prepare_pixel(const cfpt coords, fractal_point& out) const;
	virtual void plot_pixel(const int maxiter, fractal_point& iters_out) const;
};


void Mandel3::prepare_pixel(const cfpt coords, fractal_point& out) const
{
	//fvalue o_re = real(coords), o_im = imag(coords);

	// TODO: Main bulbs shortcut.

	// The first iteration is easy, 0^3 + origin = origin
	out.origin = out.point = cfpt(coords);
	out.iter = 1;
	return;
}

void Mandel3::plot_pixel(const int maxiter, fractal_point& out) const
{
	// Speed notes:
	// Don't use cfpt in the actual calculation - using straight doubles and
	// doing the complex maths by hand is about 6x faster for me.
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
			out.iterf = iter - log(log(re2 + im2)) / consts.log3;
			out.arg = atan2(z_im, z_re);
			out.nomore = true;
			return;
		}
	}
	out.iter = iter;
	out.point = cfpt(z_re,z_im);
}

Mandel3 mandel3("Mandelbrot^3");
#undef ITER3

// --------------------------------------------------------------------

class Mandel4 : public Mandelbrot {
public:
	Mandel4(std::string name_, fvalue xmin_=-3.0, fvalue xmax_=3.0, fvalue ymin_=-3.0, fvalue ymax_=3.0) : Mandelbrot(name_, xmin_, xmax_, ymin_, ymax_) {};
	~Mandel4() {};

	virtual void prepare_pixel(const cfpt coords, fractal_point& out) const;
	virtual void plot_pixel(const int maxiter, fractal_point& iters_out) const;
};


void Mandel4::prepare_pixel(const cfpt coords, fractal_point& out) const
{
	//fvalue o_re = real(coords), o_im = imag(coords);

	// TODO: Shortcuts?

	// The first iteration is easy, 0^4 + origin = origin
	out.origin = out.point = cfpt(coords);
	out.iter = 1;
	return;
}

void Mandel4::plot_pixel(const int maxiter, fractal_point& out) const
{
	// Speed notes:
	// Don't use cfpt in the actual calculation - using straight doubles and
	// doing the complex maths by hand is about 6x faster for me.
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
			out.iterf = iter - log(log(re2 + im2)) / (2*consts.log2);
			out.arg = atan2(z_im, z_re);
			out.nomore = true;
			return;
		}
	}
	out.iter = iter;
	out.point = cfpt(z_re,z_im);
}

Mandel4 mandel4("Mandelbrot^4");
#undef ITER4

