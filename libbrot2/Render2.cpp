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

void Base::process(const std::list<Plot3Chunk*>& chunks)
{
	std::list<Plot3Chunk*>::const_iterator it;
	for (it = chunks.begin() ; it != chunks.end(); it++) {
		process(**it);
	}
}

MemoryBuffer::MemoryBuffer(unsigned char *buf, int rowstride, unsigned width, unsigned height,
			const int local_inf, pixpack_format fmt, const BasePalette& pal) :
			_buf(buf), _rowstride(rowstride), _width(width), _height(height),
			_local_inf(local_inf), _fmt(fmt), _pal(pal)
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

void MemoryBuffer::process(const Plot3Chunk& chunk)
{
	(void)chunk;
	const Fractal::PointData * data = chunk.get_data();

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but gdk assumes a top-left origin.

	unsigned i,j;

	// Sanity checks
	ASSERT( chunk._offX + chunk._width <= _width );
	ASSERT( chunk._offY + chunk._height <= _height );

	for (j=0; j<chunk._height; j++) {
		unsigned char *dst = &_buf[ (chunk._offY + j) * _rowstride
		                            + chunk._offX * _pixelstep];
		const Fractal::PointData * src = &data[j*chunk._width];

		for (i=0; i<chunk._width; i++) {
			rgb pix = render_pixel(src[i], _local_inf, &_pal);
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
	}
}

PNG::PNG(unsigned width, unsigned height,
		const BasePalette& palette, int local_inf) :
		_width(width), _height(height), _local_inf(local_inf),
		_pal(palette), _png(_width, _height)
{
}

PNG::~PNG()
{
}

void PNG::process(const Plot3Chunk& chunk)
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
			_png[j+chunk._offY][i+chunk._offX] = png::rgb_pixel(pix.r, pix.g, pix.b);
		}
	}
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

PNG_AntiAliased::PNG_AntiAliased(unsigned width, unsigned height,
		const BasePalette& palette, int local_inf) :
				PNG(width, height, palette, local_inf)
{
}

PNG_AntiAliased::~PNG_AntiAliased()
{
}

void PNG_AntiAliased::process(const Plot3Chunk& chunk)
{
	const Fractal::PointData * data = chunk.get_data();

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but gdk assumes a top-left origin.

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
			_png[j+outOffY][i+outOffX] = png::rgb_pixel(aapix.r, aapix.g, aapix.b);
		}
	}
}

// Factory ??

}; // namespace Render2
