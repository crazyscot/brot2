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

typedef struct colour colour;

struct colour {
	guchar r,g,b;
};

/* Palette generation function.
 * Will be called repeatedly with 0 <= step <= nsteps, expected to be
 * idempotent.
 * Generators are usually expected to set r=g=b=0 if step==nsteps.
 */
typedef colour (*PaletteGenerator)(int step, int nsteps);

class DiscretePalette {

public:
	// Constructor does *NOT* register; caller may do so at their choice.
	DiscretePalette(int newsize, string newname);

	// Destructor will not deregister either, on the grounds that it
	// shouldn't ever be called on a registered palette!
	virtual ~DiscretePalette();

	const std::string name;
	const int size; // number of elements in table
	colour *table; // memory owned by constructor

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

// XXX operator[]
// XXX set up initial palette from the C via the factory and TEST IT.


// TODO: ContinuousPalette extends DiscretePalette and
// allows the results to be cached for discrete use?

#endif /* PALETTE_H_ */
