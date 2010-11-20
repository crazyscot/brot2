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

rgb white(255,255,255);
rgb black(0,0,0);

std::ostream& operator<<(std::ostream &stream, rgb o) {
	  stream << "rgb(" << (int)o.r << "," << (int)o.g << "," << (int)o.b << ")";
	  return stream;
}

std::ostream& operator<<(std::ostream &stream, rgbf o) {
	  stream << "rgbf(" << o.r << "," << o.g << "," << o.b << ")";
	  return stream;
}

map<string,DiscretePalette*> DiscretePalette::registry;


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

/////////////////////////////////////////////////////////////////////////

class Kaleidoscopic : DiscretePalette {
public:
	Kaleidoscopic(string name, int n) : DiscretePalette(name,n) {};
	virtual rgb get(const fractal_point &pt) const {
		// This one jumps at random around the hue space.
			hsv h(255*cos(pt.iter), 224, 224);
			rgb r(h);
		#if DEBUG_DUMP_HSV
			cout << h << " --> " << r << endl;
		#endif
			return r;
	};
};

Kaleidoscopic kaleido32("Kaleidoscopic", 32);

class Rainbow : DiscretePalette {
public:
	Rainbow(string name, int n) : DiscretePalette(name, n) {};
	virtual rgb get(const fractal_point &pt) const {
		// This is a continuous "gradient" around the Hue wheel.
		hsv h(255*pt.iter/size, 255, 255);
		rgb r(h);
	#if DEBUG_DUMP_HSV
		cout << h << " --> " << r << endl;
	#endif
		return r;
	};
};

Rainbow grad32("Rainbow", 32);

class PastelSalad : DiscretePalette {
public:
	PastelSalad(string name, int n) : DiscretePalette(name, n) {};
	virtual rgb get(const fractal_point &pt) const {
		return rgb((size-pt.iter)*255/size,
					144,
					255*cos(pt.iter));

	};
};

PastelSalad pastel32("Pastel Salad", 32);

class SawtoothGradient : public DiscretePalette {
protected:
	rgbf point1, point2;
public:
	SawtoothGradient(string nam, int n, const rgbf p1, const rgbf p2) : DiscretePalette(nam,n), point1(p1), point2(p2) {};

	virtual rgb get(const fractal_point &pt) const {
		// I tried a sinusoid function here as well, but it didn't work so well.
		int iter = pt.iter % size;
		float tau = 2.0 * iter / size;
		if (iter > (size/2)) tau = 2.0 - tau;
		rgbf r = point1 * tau + point2 * (1.0-tau);
		return r;
	}
};

#define SAWTOOTH(id,desc,n,p1,p2) \
	SawtoothGradient id(#desc, n, p1, p2)

//SAWTOOTH(saw_red_blue, Gradient red-blue, 16, rgbf(1,0,0), rgbf(0,0,1));
//SAWTOOTH(saw_green_pink, Gradient green-pink, 16, rgbf(0,1,0), rgbf(1,0,1));
SAWTOOTH(saw_red_cyan, Red-cyan sawtooth, 16, rgbf(0,1,1), rgbf(1,0,0));
//SAWTOOTH(saw_blue_purple, Gradient blue-purple, 16, rgbf(0.5,0,0.5), rgbf(0,0.5,1));
//SAWTOOTH(saw_org_green, Gradient orange-green, 16, rgbf(1,0.56,0), rgbf(0,0.56,0));

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

/*
HUECYCLE(red_violet_64, Deep red-yellow, 64, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(red_violet_32, Mid orange, 32, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(red_violet_16, Shallow yellow, 16, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(viol_red_64, Deep red-pink, 64, hsvf(0,1,1), hsvf(1,1,1));
HUECYCLE(viol_red_32, Mid pink, 32, hsvf(0,1,1), hsvf(1,1,1));
HUECYCLE(viol_red_16, Shallow greenish, 16, hsvf(0,1,1), hsvf(1,1,1));
*/

// In fact my HSV space conversion just copes with values >1, so you can do this:
HUECYCLE(green_32, Linear rainbow, 32, hsvf(0.5,1,1), hsvf(1.5,1,1));
//HUECYCLE(green_16, Shallow rainbow, 16, hsvf(0.5,1,1), hsvf(1.5,1,1));
//HUECYCLE(green_64, Deep rainbow, 64, hsvf(0.5,1,1), hsvf(1.5,1,1));

class LogSmoothed : public SmoothPalette {
public:
	LogSmoothed(std::string name) : SmoothPalette(name) {};
	hsvf get_hsvf(const fractal_point &pt) const {
		hsvf rv;
		double t = log(pt.iterf);
		rv.h = t/2.0 + 0.5;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	}
	rgb get(const fractal_point &pt) const {
		return hsv(get_hsvf(pt));
	};
};

LogSmoothed log_smoothed("Logarithmic rainbow");

class LogSmoothedRays : public LogSmoothed {
public:
	LogSmoothedRays(std::string name) : LogSmoothed(name) {};
	rgb get(const fractal_point &pt) const {
		hsvf rv = get_hsvf(pt);
		rv.s = cos(pt.arg);
		return hsv(rv);
	};
};

LogSmoothedRays log_smoothed_with_rays("Logarithmic rainbow with rays");

class SlowLog : public SmoothPalette {
public:
	SlowLog(std::string name) : SmoothPalette(name) {};
	rgb get(const fractal_point &pt) const {
		hsvf rv;
		rv.h = 0.6+cos(log(pt.iterf)/11*M_PI);
		rv.s = 1.0;
		rv.v = 1.0;
		return hsv(rv);
	};
};

SlowLog slow_log("Slow logarithmic rainbow");
