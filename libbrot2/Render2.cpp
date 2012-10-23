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

#include <png.h>
#include <stdlib.h>
#include "Render2.h"
#include "Plot3Chunk.h"
#include "palette.h"
#include "Exception.h"

namespace Render2 {

Generic::Generic(unsigned char *buf, int rowstride, unsigned width, unsigned height,
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
	}
}

Generic::~Generic()
{
}

void Generic::process(const Plot3Chunk& chunk)
{
	(void)chunk;
	const Fractal::PointData * data = chunk.get_data();

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but gdk assumes a top-left origin.

	// need: chunk offset X,Y; pixel step

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

void Generic::process(const std::list<Plot3Chunk*>& chunks)
{
	std::list<Plot3Chunk*>::const_iterator it;
	for (it = chunks.begin() ; it != chunks.end(); it++) {
		process(**it);
	}
}

}; // namespace Render2

// Then save as PNG, or whatever ... Work in render_generic ...?
// PNGRender is a Render !
