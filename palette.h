/*
    palette.h: Discrete and continuous palette interface
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

#ifndef PALETTE_H_
#define PALETTE_H_

#include <gtk/gtk.h>
#include <string>
#include <map>
#include <iostream>

typedef struct rgb colour;

class hsv {
public:
	hsv() {};
	hsv(guchar hh,guchar ss,guchar vv) : h(hh), s(ss), v(vv) {};
	guchar h,s,v;
	operator rgb();
};

class hsvf {
public:
	float h,s,v; // 0..1
	hsvf() {};
	hsvf(float hh, float ss, float vv) : h(hh), s(ss), v(vv) {};
	operator hsv();
};

std::ostream& operator<<(std::ostream &stream, hsv o);

std::ostream& operator<<(std::ostream &stream, hsvf o);

class rgb {
public:
	guchar r,g,b;
	rgb () {};
	rgb (guchar rr, guchar gg, guchar bb) : r(rr), g(gg), b(bb) {};
};

std::ostream& operator<<(std::ostream &stream, rgb o);

class rgbf {
protected:
	inline void clip() {
		if (r < 0.0) r=0.0;
		else if (r > 1.0) r=1.0;
		if (g < 0.0) g=0.0;
		else if (g > 1.0) g=1.0;
		if (b < 0.0) b=0.0;
		else if (b > 1.0) b=1.0;
	}
public:
	float r,g,b; // 0..1
	rgbf (const rgbf& i) : r(i.r), g(i.g), b(i.b) {};
	rgbf (rgb i) {
		r = i.r/255.0;
		g = i.g/255.0;
		b = i.b/255.0;
		clip();
	};
	rgbf (float rr, float gg, float bb) : r(rr), g(gg), b(bb) {};
	operator rgb() {
		rgb rv;
		rv.r = 255*r;
		rv.g = 255*g;
		rv.b = 255*b;
		return rv;
	}
	rgbf operator+ (const rgbf &t) const {
		rgbf rv(*this);
		rv.r += t.r;
		rv.g += t.g;
		rv.b += t.b;
		rv.clip();
		return rv;
	}
	rgbf operator* (float f) const {
		rgbf rv(*this);
		rv.r *= f;
		rv.g *= f;
		rv.b *= f;
		rv.clip();
		return rv;
	}
};

std::ostream& operator<<(std::ostream &stream, rgbf o);

/* Palette generation function.
 * Will be called repeatedly with 0 <= step <= nsteps, expected to be
 * idempotent.
 * nsteps is the number of colours, NOT the iteration limit! In other
 * words, don't set r=g=b=0 at nsteps.
 */
typedef rgb (*PaletteGenerator)(int step, int nsteps);

class DiscretePalette {

public:
	// Base constructor does *NOT* register the class; caller may do so at their choice when they've set up the table.
	DiscretePalette(int newsize, std::string newname);

	// Construction, initialisation and registration in one go.
	DiscretePalette(std::string newname, int newsize, PaletteGenerator gen_fn);

	// Destructor will deregister iff the instance was registered.
	virtual ~DiscretePalette();

	const std::string name;
	const int size; // number of elements in table
	rgb *table; // memory owned by constructor

	rgb& operator[](int i) { return table[i%size]; }
	const rgb& operator[](int i) const { return table[i%size]; }

	static std::map<std::string,DiscretePalette*> registry;

	void reg() { registry[name] = this; isRegistered = 1; }
	void dereg()
	{
		if (isRegistered)
			registry.erase(name);
		isRegistered = 0;
	}

protected:
	int isRegistered;
};

#endif /* PALETTE_H_ */
