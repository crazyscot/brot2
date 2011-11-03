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
#include "Registry.h"
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>

using namespace std;
using namespace Fractal;

#define DEBUG_DUMP_ALL 0
#define DEBUG_DUMP_HSV 0

const rgb white(255,255,255);
const rgb black(0,0,0);

std::ostream& operator<<(std::ostream &stream, rgb o) {
	  stream << "rgb(" << (int)o.r << "," << (int)o.g << "," << (int)o.b << ")";
	  return stream;
}

std::ostream& operator<<(std::ostream &stream, rgbf o) {
	  stream << "rgbf(" << o.r << "," << o.g << "," << o.b << ")";
	  return stream;
}

// HSV->RGB conversion algorithm coded up from the formula on Wikipedia.
hsvf::operator rgb() {
	if (isnan(h)) {
		return rgbf(v,v,v); // "undefined" case
	}
	if (isinf(h)) {
		assert(false); // should never happen
		return rgb(0,0,0); // eek, not a real number
	}
	float chroma = v * s, tmp;
	if ((h<0.0) || (h>=1.0)) h = modff(h,&tmp);
	// h': which part of the hexcone does h fall into?
	float hp = h*6.0;
	float hm = hp;
	while (hm>2.0) hm -= 2.0; // so hm = hp mod 2
	float x;
	if (hm<1.0)	x=hm;
	else		x=2.0-hm;
	x *= chroma;

	rgbf rv;
	switch((int)hp) {
	case 0: case 6:
		rv = rgbf(chroma,x,0); break;
	case 1:
		rv = rgbf(x,chroma,0); break;
	case 2:
		rv = rgbf(0,chroma,x); break;
	case 3:
		rv = rgbf(0,x,chroma); break;
	case 4:
		rv = rgbf(x,0,chroma); break;
	case 5:
		rv = rgbf(chroma,0,x); break;
	}
	float m = v - chroma;
	rv.r += m; rv.g += m; rv.b += m;
	return rv;
}

std::ostream& operator<<(std::ostream &stream, hsvf o) {
	  stream << "hsvf(" << o.h << "," << o.s << "," << o.v << ")";
	  return stream;
}

/////////////////////////////////////////////////////////////////////////

class Kaleidoscopic : public DiscretePalette {
public:
	Kaleidoscopic(int n) : DiscretePalette(n) {};
	virtual rgb get(const PointData &pt) const {
		// This one jumps at random around the hue space.
			hsvf h(0.5+cos(pt.iter)/2.0, 0.87, 0.87);
			rgb r(h);
		#if DEBUG_DUMP_HSV
			cout << h << " --> " << r << endl;
		#endif
			return r;
	};
};

class Rainbow : public DiscretePalette {
public:
	Rainbow(int n) : DiscretePalette(n) {};
	virtual rgb get(const PointData &pt) const {
		// This is a continuous "gradient" around the Hue wheel.
		double n = (double)pt.iter / size;
		double tmp;
		n = modf(n,&tmp);
		hsvf h(n, 1, 1);
		rgb r(h);
	#if DEBUG_DUMP_HSV
		cout << h << " --> " << r << endl;
	#endif
		return r;
	};
};

class PastelSalad : public DiscretePalette {
public:
	PastelSalad(int n) : DiscretePalette(n) {};
	virtual rgb get(const PointData &pt) const {
		return rgb((size-pt.iter)*255/size,
					144,
					255*cos(pt.iter));

	};
};

class SawtoothGradient : public DiscretePalette {
protected:
	rgbf point1, point2;
public:
	SawtoothGradient(int n, const rgbf p1, const rgbf p2) : DiscretePalette(n), point1(p1), point2(p2) {};

	virtual rgb get(const PointData &pt) const {
		// I tried a sinusoid function here as well, but it didn't work so well.
		int iter = pt.iter % size;
		float tau = 2.0 * iter / size;
		if (iter > (size/2)) tau = 2.0 - tau;
		rgbf r = point1 * tau + point2 * (1.0-tau);
		return r;
	}
};

//#define SAWTOOTH(id,desc,n,p1,p2) SawtoothGradient id(#desc, n, p1, p2)

//SAWTOOTH(saw_red_blue, Gradient red-blue, 16, rgbf(1,0,0), rgbf(0,0,1));
//SAWTOOTH(saw_green_pink, Gradient green-pink, 16, rgbf(0,1,0), rgbf(1,0,1));
//SAWTOOTH(saw_red_cyan, Red-cyan sawtooth, 16, rgbf(0,1,1), rgbf(1,0,0));
//SAWTOOTH(saw_blue_purple, Gradient blue-purple, 16, rgbf(0.5,0,0.5), rgbf(0,0.5,1));
//SAWTOOTH(saw_org_green, Gradient orange-green, 16, rgbf(1,0.56,0), rgbf(0,0.56,0));

SimpleRegistry<DiscretePalette> DiscretePalette::all;
int DiscretePalette::base_registered=0;

void DiscretePalette::register_base() {
	if (base_registered) return;
	all.reg("Kaleidoscopic", new Kaleidoscopic(32));
	all.reg("Rainbow", new Rainbow(32));
	all.reg("Pastel Salad", new PastelSalad(32));
	all.reg("Red-cyan sawtooth", new SawtoothGradient(16, rgbf(0,1,1), rgbf(1,0,0)));
	base_registered = 1;
}

////////////////////////////////////////////////////////////////

class HueCycle : public SmoothPalette {
public:
	const double wrap;
	const hsvf pointa, pointb; // Points to smooth between
	HueCycle(float wrap, hsvf pa, hsvf pb) : SmoothPalette(), wrap(wrap), pointa(pa), pointb(pb) {};
	rgb get(const PointData &pt) const {
		float tau,tmp;
		tau = pt.iterf / wrap;
		tau = modff(tau,&tmp);
		float taup = 1.0 - tau;
		hsvf rv (pointa.h*tau + pointb.h*taup,
				 pointa.s*tau + pointb.s*taup,
				 pointa.v*tau + pointb.v*taup);
		return rv;
	};
};

/*
#define HUECYCLE(id,name,wrap,a,b) \
	HueCycle id(#name, wrap, a, b)

HUECYCLE(red_violet_64, Deep red-yellow, 64, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(red_violet_32, Mid orange, 32, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(red_violet_16, Shallow yellow, 16, hsvf(1,1,1), hsvf(0,1,1));
HUECYCLE(viol_red_64, Deep red-pink, 64, hsvf(0,1,1), hsvf(1,1,1));
HUECYCLE(viol_red_32, Mid pink, 32, hsvf(0,1,1), hsvf(1,1,1));
HUECYCLE(viol_red_16, Shallow greenish, 16, hsvf(0,1,1), hsvf(1,1,1));
*/

// In fact my HSV space conversion just copes with values >1, so you can do this:
//HUECYCLE(green_32, Linear rainbow, 32, hsvf(0.5,1,1), hsvf(1.5,1,1));
//HUECYCLE(green_16, Shallow rainbow, 16, hsvf(0.5,1,1), hsvf(1.5,1,1));
//HUECYCLE(green_64, Deep rainbow, 64, hsvf(0.5,1,1), hsvf(1.5,1,1));

class LogSmoothed : public SmoothPalette {
public:
	LogSmoothed() : SmoothPalette() {};
	hsvf get_hsvf(const PointData &pt) const {
		hsvf rv;
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		double t = log(f);
		rv.h = t/2.0 + 0.5;
		// Arbitrary scaling, chosen by accident but turns out to
		// provide slightly less eye-bleeding colours on the initial plot.
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	}
	rgb get(const PointData &pt) const {
		return get_hsvf(pt);
	};
};

class LogSmoothedRays : public LogSmoothed {
public:
	LogSmoothedRays() : LogSmoothed() {};
	rgb get(const PointData &pt) const {
		hsvf rv = get_hsvf(pt);
		rv.s = 0.5 + cos(pt.arg) / 2.0;
		return rv;
	};
};

class FastLogSmoothed : public SmoothPalette {
public:
	FastLogSmoothed() : SmoothPalette() {};
	hsvf get_hsvf(const PointData &pt) const {
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		hsvf rv;
		rv.h = log(f) + 0.5;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	}
	rgb get(const PointData &pt) const {
		return get_hsvf(pt);
	};
};

class SinLogSmoothed : public SmoothPalette {
public:
	SinLogSmoothed() : SmoothPalette() {};
	hsvf get_hsvf(const PointData &pt) const {
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		double t = log(f);
		hsvf rv;
		rv.h = sin(t)/2.0 + 0.5;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	}
	rgb get(const PointData &pt) const {
		return get_hsvf(pt);
	};
};

class SlowSineLog : public SmoothPalette {
public:
	SlowSineLog() : SmoothPalette() {};
	rgb get(const PointData &pt) const {
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		double t = log(f);
		hsvf rv;
		rv.h = 0.5 + sin(t/3.0*M_PI)/2.0;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	};
};

class FastSineLog : public SmoothPalette {
public:
	FastSineLog() : SmoothPalette() {};
	rgb get(const PointData &pt) const {
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		double t = log(f);
		hsvf rv;
		rv.h = 0.5+sin(t*M_PI)/2.0;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	};
};

class CosLogSmoothed : public SmoothPalette {
public:
	CosLogSmoothed() : SmoothPalette() {};
	hsvf get_hsvf(const PointData &pt) const {
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		double t = log(f);
		hsvf rv;
		rv.h = cos(t)/2.0 + 0.5;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	}
	rgb get(const PointData &pt) const {
		return get_hsvf(pt);
	};
};

class SlowCosLog : public SmoothPalette {
public:
	SlowCosLog() : SmoothPalette() {};
	rgb get(const PointData &pt) const {
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		double t = log(f);
		hsvf rv;
		rv.h = 0.5 + cos(t/3.0*M_PI)/2.0;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	};
};

class FastCosLog : public SmoothPalette {
public:
	FastCosLog() : SmoothPalette() {};
	rgb get(const PointData &pt) const {
		float f = pt.iterf;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		double t = log(f);
		hsvf rv;
		rv.h = 0.5+cos(t*M_PI)/2.0;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	};
};

SimpleRegistry<SmoothPalette> SmoothPalette::all;
int SmoothPalette::base_registered=0;

void SmoothPalette::register_base() {
	if (base_registered) return;
	base_registered = 1;
	all.reg("Linear rainbow", new HueCycle(32, hsvf(0.5,1,1), hsvf(1.5,1,1)));

	all.reg("Logarithmic rainbow", new LogSmoothed());
	all.reg("Logarithmic rainbow  with rays", new LogSmoothedRays());
// nasty hack: two spaces to sort this one next to the plain log rainbow.
	all.reg("Logarithmic rainbow (steep)", new FastLogSmoothed());
/* What to call it? I originally called it _fast_, but it could mislead as
 * it doesn't make the plot any faster; the gradient is _steeper_ perhaps? */


	all.reg("sin(log)", new SinLogSmoothed());
	all.reg("sin(log) shallow", new SlowSineLog());
	all.reg("sin(log) steep", new FastSineLog());

	all.reg("cos(log)", new CosLogSmoothed());
	all.reg("cos(log) shallow", new SlowCosLog());
	all.reg("cos(log) steep", new FastCosLog());
}
