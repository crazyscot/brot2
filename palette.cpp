/*
    palette.c: Discrete and continuous palette interface
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

#include "palette.h"

map<string,DiscretePalette*> DiscretePalette::registry;

DiscretePalette::DiscretePalette(int n, string nam) : name(nam), size(n) {
	table = new colour[size];
	isRegistered = 0;
}

DiscretePalette::~DiscretePalette() {
	delete[] table;
}

DiscretePalette* DiscretePalette::factory(string name, int size, PaletteGenerator genone)
{
	DiscretePalette * rv = new DiscretePalette(size,name);
	int i;
	for (i=0; i<size; i++)
		rv->table[i] = genone(i, size);
	return rv;
}

static colour generate_greenish(int step, int nsteps) {
	colour rv;
	rv.r = step*255/nsteps;
	rv.g = (nsteps-step)*255/nsteps;
	rv.b = step*255/nsteps;
	return rv;
}

void init_discretes() {
	DiscretePalette * greenish = DiscretePalette::factory("greenish32", 32, generate_greenish);
	greenish->reg();
}
