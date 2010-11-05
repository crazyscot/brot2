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

using namespace std;

typedef struct rgb colour;

class hsv {
public:
	hsv() {};
	hsv(guchar hh,guchar ss,guchar vv) : h(hh), s(ss), v(vv) {};
	guchar h,s,v;
	operator rgb();
};

class rgb {
public:
	guchar r,g,b;
	rgb () {};
	rgb (guchar rr, guchar gg, guchar bb) : r(rr), g(gg), b(bb) {};
};

/* Palette generation function.
 * Will be called repeatedly with 0 <= step <= nsteps, expected to be
 * idempotent.
 * nsteps is the number of colours, NOT the iteration limit! In other
 * words, don't set r=g=b=0 at nsteps.
 */
typedef rgb (*PaletteGenerator)(int step, int nsteps);

class DiscretePalette {

public:
	// Constructor does *NOT* register; caller may do so at their choice.
	DiscretePalette(int newsize, string newname);

	// Destructor will not deregister either, on the grounds that it
	// shouldn't be called on a registered palette... right?
	virtual ~DiscretePalette();

	const std::string name;
	const int size; // number of elements in table
	rgb *table; // memory owned by constructor

	rgb& operator[](int i) { return table[i%size]; }
	const rgb& operator[](int i) const { return table[i%size]; }

	static map<string,DiscretePalette*> registry;

	void reg() { registry[name] = this; isRegistered = 1; }
	void dereg()
	{
		if (isRegistered)
			registry.erase(name);
		isRegistered = 0;
	}

	static DiscretePalette* factory(string name, int size, PaletteGenerator gen);

protected:
	int isRegistered;
};

#endif /* PALETTE_H_ */
