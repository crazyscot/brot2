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
#include <map>
#include <iostream>
#include "Registry.h"
#include "config.h"
#include "FractalMaths.h"

namespace Fractal {

////////////////////////////////////////////////////////////////////////////

class PointData {
public:
	int iter; // Current number of iterations this point has seen, or -1 for "infinity"
	Point origin; // Original value of this point
	Point point; // Current value of the point. Not valid if iter<0.
	bool nomore; // When true, this pixel plays no further part - may also mean "infinite".
	float iterf; // smooth iterations count (only valid the pixel has nomore)
	static const float ITERF_LOW_CLAMP; // lowest possible iterf (they will be clamped to this value if lower)
	float arg; // argument of final point (only computed after pixel has nomore)

	PointData() : iter(0), origin(Point(0,0)), point(Point(0,0)), nomore(false), iterf(0), arg(0) {};
	inline void mark_infinite() {
		iter = -1;
		iterf = -1;
		nomore = true;
	};
};

class FractalImpl;

class FractalCommon {
public:
	// Master registry of all known fractals
	static SimpleRegistry<FractalImpl> registry;
	// Call on startup to load the base fractals.
	static void load_base();

	// Unloads the fractal registry, if this is somehow useful (e.g. valgrind tests)
	static void unload_registry();

	// What is the most appropriate maths type to use for this pixel size?
	// Returns v_max if nothing suits.
	static Maths::MathsType select_maths_type(Fractal::Value pixsize);
	// What is the most appropriate maths type to use for this plot?
	// Returns v_max if nothing suits.
	static Maths::MathsType select_maths_type(Fractal::Point plot_size, unsigned width, unsigned height);
protected:
	// Set when the base set has been loaded.
	static bool base_loaded;
};

/*
 * Base fractal definition. An instance knows all about a fractal _type_
 * but nothing about an individual _plot_ of it (meta-instance?)
 */
class FractalImpl {
public:
	FractalImpl(std::string _name, std::string _desc,
			Value xmin_, Value xmax_, Value ymin_, Value ymax_, int sortorder_=100) :
				name(_name), description(_desc), xmin(xmin_), xmax(xmax_), ymin(ymin_), ymax(ymax_), sortorder(sortorder_), isRegistered(false)
	{
		reg();
	};
	virtual ~FractalImpl() {
		dereg();
	};

	// Human-readable information
	const std::string name;
	const std::string description;

	const Value xmin, xmax, ymin, ymax; // Maximum useful complex area

	/* Fractal menu sort order group.
	 * Items with the same group are sorted together,
	 * then alphabetically within the group.
	 */
	const unsigned sortorder;

	/* Pixel initialisation. This is supposed to be quick and straightforward,
	 * setting up for the first iteration and performing any shortcut checks.
	 */
	virtual void prepare_pixel(const Point coords, PointData& out) const = 0;

	/* Pixel plotting. This is the slow function; it should run only up to maxiter.
	 * It's up to the fractal what happens if a pixel reaches maxiter; in the
	 * general case the nomore flag ought _not_ to be set in case this is a
	 * multi-pass adaptive run.
	 * If a pixel escapes, perform any final computation on the result and set up
	 * _out_ accordingly.
	 */
	virtual void plot_pixel(const int maxiter, PointData& out, Maths::MathsType type) const = 0;

private:
	bool isRegistered;
	void reg();
	void dereg();
};

}; // namespace Fractal

#endif /* FRACTAL_H_ */
