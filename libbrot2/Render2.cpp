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
		_width(width), _height(height), _local_inf(local_inf), _antialias(antialias), _pal(pal) {}

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
	// but gdk assumes a top-left origin.

	unsigned i,j;

	// Sanity checks
	ASSERT( chunk._offX + chunk._width <= _width );
	ASSERT( chunk._offY + chunk._height <= _height );

	for (j=0; j<chunk._height; j++) {
		const Fractal::PointData * src = &data[j*chunk._width];

		for (i=0; i<chunk._width; i++) {
			rgb pix = render_pixel(src[i], _local_inf, &_pal);
			pixel_done(i+chunk._offX, j+chunk._offY, pix);
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

	std::vector<rgb> allpix;

	ASSERT( chunk._width % 2 == 0);
	ASSERT( chunk._height % 2 == 0);
	ASSERT( chunk._offX + chunk._width <= _width*2 );
	ASSERT( chunk._offY + chunk._height <= _height*2 );
	ASSERT( outOffX + outW <= _width );
	ASSERT( outOffY + outH <= _height);

	for (j=0; j<outH; j++) {
		for (i=0; i<outW; i++) {
			allpix.clear();
			rgb pix;

			const Fractal::PointData * base = &data[2*j*chunk._width];
			pix = render_pixel(base[2*i], _local_inf, &_pal);
			allpix.push_back(pix);
			pix = render_pixel(base[2*i+1], _local_inf, &_pal);
			allpix.push_back(pix);

			base = &data[1 + 2*j*chunk._width];
			pix = render_pixel(base[2*i], _local_inf, &_pal);
			allpix.push_back(pix);
			pix = render_pixel(base[2*i+1], _local_inf, &_pal);
			allpix.push_back(pix);

			rgb aapix = antialias_pixel(allpix);
			pixel_done(i+outOffX, j+outOffY, aapix);
		}
	}
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
		THROW(Exception,"Unhandled pixpack format "+(int)fmt);
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
