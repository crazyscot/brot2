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

namespace Fractal {

// Every source file that declares fractals must have a load_ function.
// They are called in Fractal.cpp.
void load_Mandelbrot();
void load_Mandelbar();

// Maths types etc

typedef long double Value; // short for "fractal value"
typedef std::complex<Value> Point; // "complex fractal point"
#define MINIMUM_PIXEL_SIZE ((Value)0.000000000000000444089209850062616169452667236328125)
// This is 2^-51 : specific to long double; adjust for any future change.
#define MAXIMAL_DECIMAL_PRECISION 15
// How many decimal digits do you need to show this and ideally not hit artefacts?

class Consts {
public:
	static const Value log2;
	static const Value log3;
	static const Value log4;
	static const Value log5;
};

class PointData {
public:
	int iter; // Current number of iterations this point has seen, or -1 for "infinity"
	Point origin; // Original value of this point
	Point point; // Current value of the point. Not valid if iter<0.
	bool nomore; // When true, this pixel plays no further part - may also mean "infinite".
	float iterf; // smooth iterations count (only valid the pixel has nomore)
	float arg; // argument of final point (only computed after pixel has nomore)

	PointData() : iter(0), origin(Point(0,0)), point(Point(0,0)), nomore(false), iterf(0), arg(0) {};
	inline void mark_infinite() {
		iter = -1;
		iterf = -1;
		nomore = true;
	};
};

class FractalImpl;

class FractalRegistry {
public:
	static std::map<std::string,FractalImpl*>& registry();
private:
	static FractalRegistry* _instance;
	std::map<std::string,FractalImpl*> _registry;
};

// Base fractal definition. An instance knows all about a fractal _type_
// but nothing about an individual _plot_ of it (meta-instance?)
class FractalImpl {
public:
	FractalImpl(std::string name_, std::string desc_, Value xmin_, Value xmax_, Value ymin_, Value ymax_, int sortorder_=100) : name(name_), description(desc_), xmin(xmin_), xmax(xmax_), ymin(ymin_), ymax(ymax_), sortorder(sortorder_), isRegistered(false)
	{
		reg();
	};
	virtual ~FractalImpl() {
		dereg();
	};


	std::string name; // Human-readable
	std::string description; // Human-readable
	Value xmin, xmax, ymin, ymax; // Maximum useful complex area

	/* Pixel initialisation. This is supposed to be quick and straightforward,
	 * setting up for the first iteration and performing any shortcut checks.
	 * The pixel we are interested in.
	 */
	virtual void prepare_pixel(const Point coords, PointData& out) const = 0;

	/* Pixel plotting. This is the slow function; it should run only up to maxiter.
	 * It's up to the fractal what happens if a pixel reaches maxiter; in the
	 * general case the nomore flag ought _not_ to be set in case this is a
	 * multi-pass adaptive run.
	 * If a pixel escapes, perform any final computation on the result and set up
	 * _out_ accordingly.
	 */
	virtual void plot_pixel(const int maxiter, PointData& out) const = 0;

	/* Fractal menu sort order group.
	 * Items with the same group are sorted together,
	 * then alphabetically within the group.
	 */
	const unsigned sortorder;

private:
	bool isRegistered;
	void reg() { FractalRegistry::registry()[name] = this; isRegistered = 1; }
	void dereg()
	{
		if (isRegistered)
			FractalRegistry::registry().erase(name);
		isRegistered = false;
	}
};

}; // namespace Fractal

#endif /* FRACTAL_H_ */
