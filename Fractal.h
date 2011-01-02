/*
    Fractal.h: Fractal computation interface and instances
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

#ifndef FRACTAL_H_
#define FRACTAL_H_

#include <string>
#include <complex>
#include <map>
#include <iostream>

typedef long double fvalue; // short for "fractal value"
typedef std::complex<fvalue> cfpt; // "complex fractal point"
#define MINIMUM_PIXEL_SIZE ((fvalue)0.000000000000000444089209850062616169452667236328125)
// This is 2^-51 : specific to long double; adjust for any future change.
#define MAXIMAL_DECIMAL_PRECISION 15
// How many decimal digits do you need to show this and ideally not hit artefacts?

class fractal_point {
public:
	int iter; // Current number of iterations this point has seen, or -1 for "infinity"
	cfpt origin; // Original value of this point
	cfpt point; // Current value of the point. Not valid if iter<0.
	bool nomore; // When true, this pixel plays no further part - may also mean "infinite".
	float iterf; // smooth iterations count (only valid the pixel has nomore)
	float arg; // argument of final point (only computed after pixel has nomore)

	fractal_point() : iter(0), origin(cfpt(0,0)), point(cfpt(0,0)), nomore(false), iterf(0), arg(0) {};
	inline void mark_infinite() {
		iter = -1;
		iterf = -1;
		nomore = true;
	};
};

class _consts {
public:
	static const fvalue log2;
	static const fvalue log3;
	static const fvalue log5;
};

// Base fractal definition. An instance knows all about a fractal _type_
// but nothing about an individual _plot_ of it (meta-instance?)
class Fractal {
public:
	Fractal(std::string name_, std::string desc_, fvalue xmin_, fvalue xmax_, fvalue ymin_, fvalue ymax_) : name(name_), description(desc_), xmin(xmin_), xmax(xmax_), ymin(ymin_), ymax(ymax_), isRegistered(false)
	{
		reg();
	};
	virtual ~Fractal() {
		dereg();
	};


	std::string name; // Human-readable
	std::string description; // Human-readable
	fvalue xmin, xmax, ymin, ymax; // Maximum useful complex area

	/* Pixel initialisation. This is supposed to be quick and straightforward,
	 * setting up for the first iteration and performing any shortcut checks.
	 * The pixel we are interested in.
	 */
	virtual void prepare_pixel(const cfpt coords, fractal_point& out) const = 0;

	/* Pixel plotting. This is the slow function; it should run only up to maxiter.
	 * It's up to the fractal what happens if a pixel reaches maxiter; in the
	 * general case the nomore flag ought _not_ to be set in case this is a
	 * multi-pass adaptive run.
	 * If a pixel escapes, perform any final computation on the result and set up
	 * _out_ accordingly.
	 */
	virtual void plot_pixel(const int maxiter, fractal_point& out) const = 0;

	static std::map<std::string,Fractal*> registry;

private:
	bool isRegistered;
	void reg() { registry[name] = this; isRegistered = 1; }
	void dereg()
	{
		if (isRegistered)
			registry.erase(name);
		isRegistered = false;
	}
};

#endif /* FRACTAL_H_ */
