/*
    Fractal: Core fractal computation interface bits
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

#include <string>
#include <map>
#include <glib/gmacros.h>
#include "Fractal.h"

using namespace Fractal;

const Value Consts::log2 = logl(2.0);
const Value Consts::log3 = logl(3.0);
const Value Consts::log4 = logl(4.0);
const Value Consts::log5 = logl(5.0);

const float Fractal::PointData::ITERF_LOW_CLAMP = 0.0001;

SimpleRegistry<FractalImpl> Fractal::FractalCommon::registry;
bool Fractal::FractalCommon::base_loaded;

// Called during initialisation time, so no need to worry about thread-safety.
void Fractal::FractalCommon::load_base() {
	if (base_loaded) return;
	base_loaded = true;
	load_Mandelbrot();
	load_Mandelbar();
	load_Mandeldrop();
	load_Misc();
}

const char* const Fractal::value_names[] = {
	"double",
	"long double",
};

value_e Fractal::FractalCommon::select_maths_type(Value pixsize) {
	value_e arithtype = v_max;
	if (pixsize >= value_traits<double>::min_pixel_size())
		arithtype = v_double;
	else if (pixsize >= value_traits<long double>::min_pixel_size())
		arithtype = v_long_double;
	return arithtype;
}

value_e Fractal::FractalCommon::select_maths_type(Point plot_size, unsigned width, unsigned height) {
	Value pixsize = MAX(real(plot_size),imag(plot_size)) / (Value)MAX(width,height);
	return select_maths_type(pixsize);
}
