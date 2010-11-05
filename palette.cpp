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
#include <math.h>

map<string,DiscretePalette*> DiscretePalette::registry;

DiscretePalette::DiscretePalette(int n, string nam) : name(nam), size(n), isRegistered(0) {
	table = new rgb[size];
}

DiscretePalette::~DiscretePalette() {
	if (table) delete[] table;
	dereg();
}

DiscretePalette::DiscretePalette(string nam, int n, PaletteGenerator gen_fn) : name(nam), size(n) {
	table = new rgb[size];
	for (int i=0; i<size; i++)
		table[i] = gen_fn(i, size);
	reg();
}

static rgb generate_greenish(int step, int nsteps) {
	return rgb(step*255/nsteps,
			   (nsteps-step)*255/nsteps,
			   step*255/nsteps);
}

static rgb generate_redish(int step, int nsteps) {
	return rgb((nsteps-step)*255/nsteps,
			   step*255/nsteps,
			   step*255/nsteps);
}

static rgb generate_blueish(int step, int nsteps) {
	return rgb((nsteps-step)*128/nsteps,
				step*127/nsteps,
				128+step*127/nsteps);
}

// HSV->RGB conversion algorithm found under a rock on the 'net.
hsv::operator rgb() {
	if (isnan(h)) return rgb(v,v,v); // "undefined" case
	float hh = 6.0*h/255.0, ss = s/255.0, vv = v/255.0, m, n, f;
	int i;

	if (hh == 0) hh=0.01;

    i = floor(hh);
	f = hh - i;
	if(!(i & 1)) f = 1 - f; // if i is even
	m = vv * (1 - ss);
	n = vv * (1 - ss * f);
	switch (i)
	{
	case 6:
	case 0: return rgb(v, n, m);
	case 1: return rgb(n, v, m);
	case 2: return rgb(m, v, n);
	case 3: return rgb(m, n, v);
	case 4: return rgb(n, m, v);
	case 5: return rgb(v, m, n);
	}
	return rgb(0, 0, 0);
}

static rgb generate_hsv(int step, int nsteps) {
	return hsv(255.0*step/nsteps, 255, 255);
}

static rgb generate_orange_green(int step, int nsteps) {
	return rgb((nsteps-step)*255/nsteps,
				255*acos(step/nsteps),
				255/nsteps);
}

// Static instances:
DiscretePalette green_pink("green+pink", 32, generate_greenish);
DiscretePalette blue_purple("blue-purple", 16, generate_blueish);
DiscretePalette red_cyan("red-cyan", 32, generate_redish);
DiscretePalette hsv100("strident primaries", 100, generate_hsv);
DiscretePalette orange_green("orange-green", 16, generate_orange_green);
