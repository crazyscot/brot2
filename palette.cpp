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
#include "Fractal.h"
#include <math.h>
#include <stdio.h>
#include <iostream>

using namespace std;

#define DEBUG_DUMP_ALL 0
#define DEBUG_DUMP_HSV 0

std::ostream& operator<<(std::ostream &stream, rgb o) {
	  stream << "rgb(" << (int)o.r << "," << (int)o.g << "," << (int)o.b << ")";
	  return stream;
}

std::ostream& operator<<(std::ostream &stream, rgbf o) {
	  stream << "rgbf(" << o.r << "," << o.g << "," << o.b << ")";
	  return stream;
}

map<string,DiscretePalette*> DiscretePalette::registry;

DiscretePalette::DiscretePalette(int n, string nam) : BasePalette(nam), size(n), isRegistered(0) {
	table = new rgb[size];
}

DiscretePalette::~DiscretePalette() {
	if (table) delete[] table;
	dereg();
}

DiscretePalette::DiscretePalette(string nam, int n, PaletteGenerator gen_fn) : BasePalette(nam), size(n) {
#if DEBUG_DUMP_ALL
	cout << nam << endl;
#endif
	table = new rgb[size];
	for (int i=0; i<size; i++) {
		table[i] = gen_fn(i, size);
#if DEBUG_DUMP_ALL
		cout << i << ": " << table[i] << endl;
#endif
	}
	reg();
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
	case 0: return rgbf(vv, n,  m);
	case 1: return rgbf(n,  vv, m);
	case 2: return rgbf(m,  vv, n);
	case 3: return rgbf(m,  n,  vv);
	case 4: return rgbf(n,  m,  vv);
	case 5: return rgbf(vv, m,  n);
	}
	return rgb(0, 0, 0);
}

std::ostream& operator<<(std::ostream &stream, hsv o) {
	  stream << "hsv(" << (int)o.h << "," << (int)o.s << "," << (int)o.v << ")";
	  return stream;
}

hsvf::operator hsv() {
	return hsv(h*255.0, s*255.0, v*255.0);
}

std::ostream& operator<<(std::ostream &stream, hsvf o) {
	  stream << "hsvf(" << o.h << "," << o.s << "," << o.v << ")";
	  return stream;
}

static rgb generate_hsv(int step, int nsteps) {
	hsv h(255*cos(step), 224, 224);
	rgb r(h);
	// This one jumps at random around the hue space.
#if DEBUG_DUMP_HSV
	cout << h << " --> " << r << endl;
#endif
	return r;
}

static rgb generate_hsv2(int step, int nsteps) {
	// This is a continuous gradient.
	hsv h(255*step/nsteps, 255, 255);
	rgb r(h);
#if DEBUG_DUMP_HSV
	cout << h << " --> " << r << endl;
#endif
	return r;
}

static rgb generate_pastel1(int step, int nsteps) {
	return rgb((nsteps-step)*255/nsteps,
				144,
				255*cos(step));
}

// Static instances:
#define P(label,desc,n,gen) \
	DiscretePalette label(#desc, n, gen)

P(hsv1, Kaleidoscopic, 32, generate_hsv);
P(hsv2, Gradient RGB, 32, generate_hsv2);
P(pastel1, Pastel salad, 16, generate_pastel1);

class TwoPointGradient : public DiscretePalette {
	typedef rgb TWG_generate_fn(TwoPointGradient *t, int step);
public:
	rgbf point1, point2;
	TwoPointGradient(string nam, int n, const rgbf p1, const rgbf p2, TWG_generate_fn gen_fn) : DiscretePalette(n,nam), point1(p1), point2(p2) {
#if DEBUG_DUMP_ALL
		cout << nam << endl;
#endif
		table = new rgb[size];
		for (int i=0; i<size; i++) {
			table[i] = gen_fn(this, i);
#if DEBUG_DUMP_ALL
			cout << i << ": " << table[i] << endl;
#endif
		}
		reg();
	}
};

#if 0
// Sinusoids don't seem to work so well, the variation in gradient of sin(t)
// blurs arbitrary regions.
static rgb generate_sinusoid(TwoPointGradient *t, int step) {
	float tau = sin(M_PI * step / t->size);
	rgbf r = t->point1 * tau + t->point2 * (1.0-tau);
	return r;
}
TwoPointGradient test("test1 sinu", 16, rgbf(1,0,0), rgbf(0,0,1), generate_sinusoid);
#endif

static rgb generate_sawtooth(TwoPointGradient *t, int step) {
	float tau = 2.0 * step / t->size;
	if (step > (t->size/2)) tau = 2.0 - tau;
	rgbf r = t->point1 * tau + t->point2 * (1.0-tau);
	return r;
}

#define SAWTOOTH(id,desc,n,p1,p2) \
	TwoPointGradient id(#desc, n, p1, p2, generate_sawtooth)

SAWTOOTH(saw_red_blue, Gradient red-blue, 16, rgbf(1,0,0), rgbf(0,0,1));
SAWTOOTH(saw_green_pink, Gradient green-pink, 16, rgbf(0,1,0), rgbf(1,0,1));
SAWTOOTH(saw_red_cyan, Gradient red-cyan, 16, rgbf(0,1,1), rgbf(1,0,0));
SAWTOOTH(saw_blue_purple, Gradient blue-purple, 16, rgbf(0.5,0,0.5), rgbf(0,0.5,1));
SAWTOOTH(saw_org_green, Gradient orange-green, 16, rgbf(1,0.56,0), rgbf(0,0.56,0));

////////////////////////////////////////////////////////////////

std::map<std::string,SmoothPalette*> SmoothPalette::registry;

class HueCycle : public SmoothPalette {
public:
	const double wrap;
	const hsvf pointa, pointb; // Points to smooth between
	HueCycle(std::string name, float wrap, hsvf pa, hsvf pb) : SmoothPalette(name), wrap(wrap), pointa(pa), pointb(pb) {};
	rgb get(const fractal_point &pt) const {
		float tau = pt.iterf / wrap;
		tau -= floor(tau);
		float taup = 1.0 - tau;
		hsvf rv (pointa.h*tau + pointb.h*taup,
				 pointa.s*tau + pointb.s*taup,
				 pointa.v*tau + pointb.v*taup);
		return hsv(rv);
	};
};

#define HUECYCLE(id,name,wrap,a,b) \
	HueCycle id(#name, wrap, a, b)

HUECYCLE(red_violet_64, Deep red-yellow, 64, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(red_violet_32, Mid orange, 32, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(red_violet_16, Shallow yellow, 16, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(viol_red_64, Deep red-pink, 64, hsvf(0,1,1), hsvf(1,1,1));
HUECYCLE(viol_red_32, Mid pink, 32, hsvf(0,1,1), hsvf(1,1,1));
HUECYCLE(viol_red_16, Shallow greenish, 16, hsvf(0,1,1), hsvf(1,1,1));

// In fact my HSV space conversion just copes with values >1, so you can do this:
HUECYCLE(green_base, Mid green, 32, hsvf(0.5,1,1), hsvf(1.5,1,1));
HUECYCLE(green_16, Shallow green, 16, hsvf(0.5,1,1), hsvf(1.5,1,1));
HUECYCLE(green_64, Deep green, 64, hsvf(0.5,1,1), hsvf(1.5,1,1));

// TODO make main use a smooth pal by default.
