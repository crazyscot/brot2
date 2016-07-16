/*
    Render2.cpp: Generic rendering functions
    Copyright (C) 2011-2012 Ross Younger

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

#include <png++/png.hpp>
#include <stdlib.h>
#include "Render2.h"
#include "Plot3Chunk.h"
#include "palette.h"
#include "Exception.h"

namespace Render2 {

using namespace Plot3;

Base::Base(unsigned width, unsigned height, int local_inf, bool antialias, const BasePalette& pal) :
		_width(width), _height(height), _local_inf(local_inf), _antialias(antialias), _pal(&pal) {}

void Base::process(const Plot3Chunk& chunk)
{
	if (_antialias)
		return process_antialias(chunk);
	else
		return process_plain(chunk);
}

void Base::process(const std::list<Plot3Chunk*>& chunks)
{
	std::list<Plot3Chunk*>::const_iterator it;
	for (it = chunks.begin() ; it != chunks.end(); it++) {
		process(**it);
	}
}

void Base::process_plain(const Plot3Chunk& chunk)
{
	const Fractal::PointData * data = chunk.get_data();

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but the rest of the universe assumes a top-left origin.

	unsigned i,j;

	// Sanity checks
	ASSERT( chunk._offX + chunk._width <= _width );
	ASSERT( chunk._offY + chunk._height <= _height );

	for (j=0; j<chunk._height; j++) {
		const Fractal::PointData * src = &data[j*chunk._width];

		for (i=0; i<chunk._width; i++) {
			rgb pix = render_pixel(src[i], _local_inf, _pal);
			pixel_done(i+chunk._offX, _height-(1+j+chunk._offY), pix);
		}
	}
}

void Base::process_antialias(const Plot3Chunk& chunk)
{
	const Fractal::PointData * data = chunk.get_data();

	unsigned i,j;
	const unsigned outW = chunk._width / 2,
				   outH = chunk._height / 2,
				   outOffX = chunk._offX / 2,
				   outOffY = chunk._offY / 2;

	ASSERT( chunk._width % 2 == 0);
	ASSERT( chunk._height % 2 == 0);
	ASSERT( chunk._offX + chunk._width <= _width*2 );
	ASSERT( chunk._offY + chunk._height <= _height*2 );
	ASSERT( outOffX + outW <= _width );
	ASSERT( outOffY + outH <= _height);

	for (j=0; j<outH; j++) {
		for (i=0; i<outW; i++) {
			rgb pix[4];

			const Fractal::PointData * base = &data[2*j*chunk._width];
			pix[0] = render_pixel(base[2*i], _local_inf, _pal);
			pix[1] = render_pixel(base[2*i+1], _local_inf, _pal);

			base = &data[(1+2*j)*chunk._width];
			pix[2] = render_pixel(base[2*i], _local_inf, _pal);
			pix[3] = render_pixel(base[2*i+1], _local_inf, _pal);

			pixel_done(i+outOffX, _height-(1+j+outOffY), antialias_pixel4(pix));
		}
	}
}

void Base::fresh_local_inf(unsigned local_inf) {
	_local_inf = local_inf;
}

void Base::fresh_palette(const BasePalette& pal) {
	_pal = &pal;
}

void Base::pixel_overlay(unsigned X, unsigned Y, const rgba& other)
{
	rgb pixel;
	this->pixel_get(X,Y,pixel);
	pixel.overlay(other);
	this->pixel_done(X, Y, pixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////

MemoryBuffer::MemoryBuffer(unsigned char *buf, int rowstride, unsigned width, unsigned height,
		bool antialias, const int local_inf, pixpack_format fmt, const BasePalette& pal) :
					Base(width, height, local_inf, antialias, pal),
					_buf(buf), _rowstride(rowstride), _fmt(fmt)
{
	ASSERT(buf);
	ASSERT((unsigned)rowstride >= RGB_BYTES_PER_PIXEL * width);

	switch(_fmt) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
		_pixelstep = 4;
		break;
	case pixpack_format::PACKED_RGB_24:
		_pixelstep = 3;
		break;
	default:
		THROW(BrotFatalException,"Unhandled pixpack format "+(int)fmt);
		break;
	}
}

MemoryBuffer::~MemoryBuffer()
{
}

void MemoryBuffer::pixel_done(unsigned X, unsigned Y, const rgb& pix)
{
	unsigned char *dst = &_buf[ Y * _rowstride + X * _pixelstep];
	switch(_fmt) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
		dst[3] = 0xff;
		dst[2] = pix.r;
		dst[1] = pix.g;
		dst[0] = pix.b;
		dst += _pixelstep; // 4
		break;
		// alpha=1.0 so these cases are the same.
	case pixpack_format::PACKED_RGB_24:
		dst[0] = pix.r;
		dst[1] = pix.g;
		dst[2] = pix.b;
		dst += _pixelstep; // 3
		break;
	}
}

void MemoryBuffer::pixel_get(unsigned X, unsigned Y, rgb& pix)
{
	unsigned char *dst = &_buf[ Y * _rowstride + X * _pixelstep];
	switch(_fmt) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
		pix.r = dst[2];
		pix.g = dst[1];
		pix.b = dst[0];
		dst += _pixelstep; // 4
		break;
		// alpha=1.0 so these cases are the same.
	case pixpack_format::PACKED_RGB_24:
		pix.r = dst[0];
		pix.g = dst[1];
		pix.b = dst[2];
		dst += _pixelstep; // 3
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

PNG::PNG(unsigned width, unsigned height,
		const BasePalette& palette, int local_inf, bool antialias) :
				Base(width, height, local_inf, antialias, palette),
				_png(_width, _height)
{
}

PNG::~PNG()
{
}

void PNG::pixel_done(unsigned X, unsigned Y, const rgb& pix) {
	_png[Y][X] = png::rgb_pixel(pix.r, pix.g, pix.b);
}

void PNG::pixel_get(unsigned X, unsigned Y, rgb& pix) {
	png::rgb_pixel p = _png[Y][X];
	pix.r = p.red;
	pix.g = p.green;
	pix.b = p.blue;
}


void PNG::write(const std::string& filename)
{
	// TODO: Set png info text (software, comment) once png++ supports this.
	_png.write(filename);
}

void PNG::write(std::ostream& os)
{
	_png.write_stream<std::ostream>(os);
}

}; // namespace Render2
