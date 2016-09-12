/*
    marshal.cpp: Data struct conversion for marshalling
    Copyright (C) 2016 Ross Younger

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

#include "marshal.h"
#include <typeinfo>

namespace b2marsh {

const std::size_t LD_precision = std::numeric_limits<long double>::digits;

bool runtime_type_check(void) {
	const std::type_info& ti1 = typeid(Fractal::Value);
	const std::type_info& ti2 = typeid(long double);
	return (ti1.hash_code() == ti2.hash_code());
}

void Value2Wire(const Fractal::Value& val, b2msg::Float* wire)
{
	// FRAGILE: Relies on Fractal::Value being long double. See the runtime check above, called from tests.
	std::stringstream str;
	str.precision(LD_precision);
	str << val;
	wire->set_longdouble(str.str());
}

void Point2Wire(const Fractal::Point& pt, b2msg::Point* wire)
{
	Value2Wire(real(pt), wire->mutable_real());
	Value2Wire(imag(pt), wire->mutable_imag());
}

void Wire2Value(const b2msg::Float& wire, Fractal::Value& val)
{
	if (wire.has_longdouble()) // TODO later switch on case
		val = std::stold(wire.longdouble());
	else
		val = NAN;
}
void Wire2Point(const b2msg::Point& wire, Fractal::Point& pt)
{
	Fractal::Value re, im;
	Wire2Value(wire.real(), re);
	Wire2Value(wire.imag(), im);
	pt.real(re);
	pt.imag(im);
}


void Movie2Wire(const struct Movie::MovieInfo& mov, b2msg::Movie* wire)
{
	b2msg::PlotInfo* info = wire->mutable_info();
	info->mutable_fractal()->set_name(mov.fractal->name);
	info->mutable_palette()->set_name(mov.palette->name);
	info->set_width(mov.width);
	info->set_height(mov.height);
	info->set_hud(mov.draw_hud);
	info->set_antialias(mov.antialias);
	info->set_preview(mov.preview);

	wire->set_fps(mov.fps);

	wire->clear_frames();
	for (auto it = mov.points.begin(); it != mov.points.end(); it++) {
		b2msg::KeyFrame* kf = wire->add_frames();
		b2msg::Frame* ff = kf->mutable_frame();
		Point2Wire((*it).centre, ff->mutable_centre());
		Point2Wire((*it).size, ff->mutable_size());
		kf->set_hold_frames( (*it).hold_frames );
		kf->set_speed_zoom ( (*it).speed_zoom );
		kf->set_speed_translate( (*it).speed_translate );
	}
}

bool Wire2Movie(const b2msg::Movie& wire, struct Movie::MovieInfo& mov)
{
	const b2msg::PlotInfo& info = wire.info();
	mov.fractal = Fractal::FractalCommon::registry.get(info.fractal().name());
	mov.palette = DiscretePalette::all.get(info.palette().name());
	if (!mov.palette)
		mov.palette = SmoothPalette::all.get(info.palette().name());
	mov.width = info.width();
	mov.height = info.height();
	mov.draw_hud = info.hud();
	mov.antialias = info.antialias();
	mov.preview = info.preview();
	mov.fps = wire.fps();

	mov.points.clear();
	for (int i=0; i<wire.frames_size(); i++) {
		const b2msg::KeyFrame& kf = wire.frames(i);
		const b2msg::Frame& ff = kf.frame();
		Fractal::Point cen, siz;
		Wire2Point(ff.centre(), cen);
		Wire2Point(ff.size(), siz);
		Movie::KeyFrame mkf(cen, siz, kf.hold_frames(), kf.speed_zoom(), kf.speed_translate());
		mov.points.push_back(mkf);
	}
	return (mov.fractal && mov.palette);
}

}; // b2marsh

