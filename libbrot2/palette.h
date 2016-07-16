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

#include <string>
#include <map>
#include <iostream>
#include "Fractal.h"
#include "Registry.h"

class rgb;
class rgba;

extern const rgb white;
extern const rgb black;

class hsvf {
public:
	float h,s,v; // 0..1 (hue may be >1, in which case we take its non-integer part)
	hsvf() : h(0),s(0),v(0) {};
	hsvf(float hh, float ss, float vv) : h(hh), s(ss), v(vv) {};
	operator rgb();
};

std::ostream& operator<<(std::ostream &stream, hsvf o);

class rgba {
	public:
		unsigned char r,g,b,a;
		rgba () : r(0),g(0),b(0),a(0) {};
		rgba (unsigned char rr, unsigned char gg, unsigned char bb, unsigned char aa) : r(rr), g(gg), b(bb), a(aa) {};

		bool operator==(const rgba& other) const {
			return (r==other.r) && (g==other.g) && (b==other.b) && (a==other.a);
		}
};

std::ostream& operator<<(std::ostream &stream, const rgba& o);

class rgb {
public:
	unsigned char r,g,b;
	rgb () : r(0),g(0),b(0) {};
	rgb (unsigned char rr, unsigned char gg, unsigned char bb) : r(rr), g(gg), b(bb) {};

	bool operator==(const rgb& other) const {
		return (r==other.r) && (g==other.g) && (b==other.b);
	}

	/** Alpha-blends another pixel on top of this one */
	inline void overlay(const rgba& other) {
		if (other.a == 0) return;
		if (other.a == 255) {
			// Simple case - optimise
			r = other.r;
			g = other.g;
			b = other.b;
			return;
		}
		unsigned alpha = other.a, invalpha = 255 - other.a;
		r = (other.r * alpha) / 255 + (r * invalpha) / 255;
		g = (other.g * alpha) / 255 + (g * invalpha) / 255;
		b = (other.b * alpha) / 255 + (b * invalpha) / 255;
	}
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
	rgbf (rgb i) : r(i.r/255.0), g(i.g/255.0), b(i.b/255.0) {
		clip();
	};
	rgbf() : r(0),g(0),b(0) {};
	rgbf (float rr, float gg, float bb) : r(rr), g(gg), b(bb) {};
	operator rgb() const {
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

class BasePalette {
public:
	BasePalette(std::string& _name): name(_name) {}
	virtual ~BasePalette() {}

	virtual rgb get(const Fractal::PointData &pt) const = 0;

	const std::string name;
};


class DiscretePalette : public BasePalette {
public:
	// Construction registration in one go.
	DiscretePalette(std::string name, int n) : BasePalette(name), size(n) { };
	virtual ~DiscretePalette() { };

	const int size; // number of colours in the palette

public:
	static SimpleRegistry<DiscretePalette> all;
	static void register_base();
protected:
	static int base_registered;
};

///////////////////////////////////////////////////////////////////

class SmoothPalette : public BasePalette {
public:
	SmoothPalette(std::string name) : BasePalette(name) { };
	virtual ~SmoothPalette() { };

	virtual rgb get(const Fractal::PointData &pt) const = 0;

	static SimpleRegistry<SmoothPalette> all;
	static void register_base();

protected:
	static int base_registered;
};

#endif /* PALETTE_H_ */
