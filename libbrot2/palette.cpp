/*
    palette.c: Discrete and continuous palette interface
    Copyright (C) 2010-3 Ross Younger

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
#include "Exception.h"
#include <math.h>
#include <stdio.h>
#include <iostream>

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

std::ostream& operator<<(std::ostream &stream, const rgba& o) {
	  stream << "rgba(" << (int)o.r << "," << (int)o.g << "," << (int)o.b << "," << (int)o.a << ")";
	  return stream;
}

// HSV->RGB conversion algorithm: Originally coded up from the formula on Wikipedia, improved version (also derived from Wikipedia) converted from JavaScript found on https://github.com/mjackson/mjijackson.github.com/blob/master/2008/02/rgb-to-hsl-and-rgb-to-hsv-color-model-conversion-algorithms-in-javascript.txt
hsvf::operator rgb() {
	if (isnan(h)) {
		return rgbf(v,v,v); // "undefined" case
	}
	if (isinf(h)) {
		ASSERT(false); // should never happen
		return rgb(0,0,0); // eek, not a real number
	}
	float tmp;
	h = modff(h,&tmp); // -1..h..1
	if (h<0) h+=1; // 0..h..1
	int i = h * 6.0;
	float f = h * 6.0 - i;
	float p = v * (1.0 - s);
	float q = v * (1.0 - f * s);
	float t = v * (1.0 - (1.0 - f) * s);
	switch(i%6) {
		case 0: return rgbf(v, t, p);
		case 1: return rgbf(q, v, p);
		case 2: return rgbf(p, v, t);
		case 3: return rgbf(p, q, v);
		case 4: return rgbf(t, p, v);
		case 5: return rgbf(v, p, q);
	}
	ASSERT(false); // unreachable
}

std::ostream& operator<<(std::ostream &stream, hsvf o) {
	  stream << "hsvf(" << o.h << "," << o.s << "," << o.v << ")";
	  return stream;
}

/////////////////////////////////////////////////////////////////////////

class Kaleidoscopic : public DiscretePalette {
public:
	Kaleidoscopic(int n) : DiscretePalette("Kaleidoscopic", n) {};
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
	rgb* colours;
public:
	Rainbow(int n) : DiscretePalette("Rainbow", n) {
		colours = new rgb[n];
		for (int i=0; i<n; i++) {
			colours[i] = calculate(i);
		}
	};
	virtual ~Rainbow() { delete[] colours; }
	rgb calculate(double n) const {
		// This is a continuous "gradient" around the Hue wheel.
		double tmp;
		n = modf(n,&tmp);
		hsvf h(n, 1, 1);
		rgb r(h);
	#if DEBUG_DUMP_HSV
		cout << h << " --> " << r << endl;
	#endif
		return r;
	};
	virtual rgb get(const PointData &pt) const {
		int n = (double)pt.iter / size;
		return colours[n];
	}
};

class PastelSalad : public DiscretePalette {
public:
	PastelSalad(int n) : DiscretePalette("Pastel salad", n) {};
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
	SawtoothGradient(std::string name, int n, const rgbf p1, const rgbf p2) : DiscretePalette(name, n), point1(p1), point2(p2) {};

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

// This one is visually disturbing. Find a nice swirly fractal plot, put it on full-screen in anti-aliased mode, and watch it appear to breathe before your eyes...
class OpticalIllusion: public DiscretePalette {
	rgb pal[4];
public:
	OpticalIllusion() : DiscretePalette("Optical Illusion", 4) {
		pal[0].r = pal[0].g = pal[0].b = 0;
		pal[1].r = 150; pal[1].g = 2; pal[1].b = 198; // purple
		pal[2].r = pal[2].g = pal[2].b = 255;
		pal[3].r = 150; pal[3].g = 216; pal[3].b = 2; // lime green

	};
	virtual rgb get(const PointData &pt) const {
		return pal[pt.iter%4];
	};
};

SimpleRegistry<DiscretePalette> DiscretePalette::all;
int DiscretePalette::base_registered=0;

#define REGISTER0(cls) do { 				\
	cls* cls##impl = new cls();				\
	all.reg(cls##impl->name, cls##impl);	\
} while(0)

#define REGISTER(cls, ...) do { 			\
	cls* cls##impl = new cls(__VA_ARGS__);	\
	all.reg(cls##impl->name, cls##impl);	\
} while(0)

void DiscretePalette::register_base() {
	if (base_registered) return;
	REGISTER(Kaleidoscopic, 32);
	REGISTER(Rainbow, 32);
	REGISTER(PastelSalad, 32);
	REGISTER(SawtoothGradient, "Red-cyan sawtooth", 16, rgbf(0,1,1), rgbf(1,0,0));
	REGISTER0(OpticalIllusion);
	base_registered = 1;
}

////////////////////////////////////////////////////////////////

class HueCycle : public SmoothPalette {
public:
	const double wrap;
	const hsvf pointa, pointb; // Points to smooth between
	HueCycle(string name, float wrap, hsvf pa, hsvf pb) : SmoothPalette(name), wrap(wrap), pointa(pa), pointb(pb) {};
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
	LogSmoothed() : SmoothPalette("Logarithmic rainbow") {};
	LogSmoothed(string name) : SmoothPalette(name) {};
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

class FastLogSmoothed : public SmoothPalette {
public:
	/* What to call it? I originally called it _fast_, but it could mislead as
	 * it doesn't make the plot any faster; the gradient is _steeper_ perhaps? */
	FastLogSmoothed() : SmoothPalette("Logarithmic rainbow (steep)") {};
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
	SinLogSmoothed() : SmoothPalette("sin(log)") {};
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
	SlowSineLog() : SmoothPalette("sin(log) shallow") {};
	SlowSineLog(std::string name) : SmoothPalette(name) {};
	rgb get(const PointData &pt) const {
		hsvf rv;
		rv.h = 0.5 + sin(log(pt.iterf)/3*M_PI)/2.0;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	};
};

class FastSineLog : public SmoothPalette {
public:
	FastSineLog() : SmoothPalette("sin(log) steep") {};
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
	CosLogSmoothed() : SmoothPalette("cos(log)") {};
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
	SlowCosLog() : SmoothPalette("cos(log) shallow") {};
	SlowCosLog(std::string name) : SmoothPalette(name) {};
	rgb get(const PointData &pt) const {
		hsvf rv;
		rv.h = 0.5 + cos(log(pt.iterf)/3*M_PI)/2.0;
		rv.s = 1.0;
		rv.v = 1.0;
		return rv;
	};
};

class FastCosLog : public SmoothPalette {
public:
	FastCosLog() : SmoothPalette("cos(log) steep") {};
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

class Mandy : public SmoothPalette {
/* The colouring scheme used by Richard Kettlewell's "mandy".
 * See http://www.greenend.org.uk/rjk/mandy/ */
public:
	Mandy() : SmoothPalette("rjk.mandy") {};
	rgb get(const PointData &pt) const {
		float f = pt.iterf;
		rgb rv;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		float c = 2.0 * M_PI * sqrt(f);
		rv.r = (cos(0.2      /*  1/5 */ *c) + 1.0) * 127;
		rv.g = (cos(0.14285  /*  1/7 */ *c) + 1.0) * 127;
		rv.b = (cos(0.090909 /* 1/11 */ *c) + 1.0) * 127;
		return rv;
	};
};

class MandyBlue : public SmoothPalette {
/* A derivative of Mandy that I discovered while noodling around. */
public:
	MandyBlue() : SmoothPalette("rjk.mandy.blue") {};
	rgb get(const PointData &pt) const {
		float f = log(pt.iterf); // one log, that's all the difference
		rgb rv;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		float c = 2.0 * M_PI * sqrt(f);
		rv.r = (cos(0.2      /*  1/5 */ *c) + 1.0) * 127;
		rv.g = (cos(0.14285  /*  1/7 */ *c) + 1.0) * 127;
		rv.b = (cos(0.090909 /* 1/11 */ *c) + 1.0) * 127;
		return rv;
	};
};

class fanfBlackFade: public SmoothPalette {
/* Based on a colouring algorithm by Tony Finch.
 * See http://dotat.at/prog/mandelbrot/ */
public:
	fanfBlackFade() : SmoothPalette("fanf.black.fade") {};
	rgb get(const PointData &pt) const {
		double f = log(pt.iterf);
		rgb rv;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		rv.r = 127 * (1.0 - cos(f*1.0));
		rv.g = 127 * (1.0 - cos(f*2.0));
		rv.b = 127 * (1.0 - cos(f*3.0));
		return rv;
	};
};

class fanfWhiteFade: public SmoothPalette {
/* Based on a colouring algorithm by Tony Finch.
 * See http://dotat.at/prog/mandelbrot/ */
public:
	fanfWhiteFade() : SmoothPalette("fanf.white.fade") {};
	rgb get(const PointData &pt) const {
		double f = log(pt.iterf);
		rgb rv;
		if (f < Fractal::PointData::ITERF_LOW_CLAMP)
			f = Fractal::PointData::ITERF_LOW_CLAMP;
		rv.r = 127 * (1.0 + cos(f*2.0));
		rv.g = 127 * (1.0 + cos(f*1.5));
		rv.b = 127 * (1.0 + cos(f*1.0));
		return rv;
	};
};

SimpleRegistry<SmoothPalette> SmoothPalette::all;
int SmoothPalette::base_registered=0;

// same REGISTER macro as for Discretes

void SmoothPalette::register_base() {
	if (base_registered) return;
	base_registered = 1;
	REGISTER(HueCycle, "Linear rainbow", 32, hsvf(0.5,1,1), hsvf(1.5,1,1));

	REGISTER0(LogSmoothed);
	REGISTER0(FastLogSmoothed);

	REGISTER0(SinLogSmoothed);
	REGISTER0(FastSineLog);
	REGISTER0(SlowSineLog);
	REGISTER0(CosLogSmoothed);
	REGISTER0(FastCosLog);
	REGISTER0(SlowCosLog);

	REGISTER0(Mandy);
	REGISTER0(MandyBlue);

	REGISTER0(fanfBlackFade);
	REGISTER0(fanfWhiteFade);
}
