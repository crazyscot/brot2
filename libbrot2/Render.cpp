/*
    render.cpp: Generic rendering functions
    Copyright (C) 2011 Ross Younger

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
#include <assert.h>
#include <stdlib.h>
#include "Render.h"
#include "Plot2.h"

/*
 * The actual work of turning an array of fractal_points - maybe an antialiased
 * set - into a packed array of pixels to a given format.
 *
 * buf: Where to put the data. This should be at least
 * (rowstride * rctx->height) bytes long.
 *
 * rowstride: the size of an output row, in bytes. In other words the byte
 * offset from one row to the next - which may be different from
 * (bytes per pixel * rwidth) if any padding is required.
 *
 * local_inf: the local plot's current idea of infinity.
 * (N.B. -1 is always treated as infinity.)
 *
 * fmt: The byte format to use. This may be a CAIRO_FORMAT_* or our
 * internal PACKED_RGB_24 (used for png output).
 *
 * Returns: True if the render completed, false if the plot disappeared under
 * our feet (typically by the user doing something to cause us to render
 * afresh).
 */
bool Render::render_generic(unsigned char *buf, const int rowstride, const int local_inf, pixpack_format fmt, Plot2& plot, unsigned rwidth, unsigned rheight, unsigned /*antialias*/factor, BasePalette& pal)
{
	assert(buf);
	assert((unsigned)rowstride >= RGB_BYTES_PER_PIXEL * rwidth);

	const Fractal::PointData * data = plot.get_data();
	if (!data) return false; // Oops, disappeared under our feet

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but gdk assumes a top-left origin.

	const Fractal::PointData ** srcs = new const Fractal::PointData* [ factor ];
	unsigned i,j;
	for (j=0; j<rheight; j++) {
		unsigned char *dst = &buf[j*rowstride];
		const unsigned src_idx = (rheight - j - 1) * factor;
		for (unsigned k=0; k < factor; k++) {
			srcs[k] = &data[plot.width * (src_idx+k)];
		}

		for (i=0; i<rwidth; i++) {
			unsigned rr=0, gg=0, bb=0; // Accumulate the result

			for (unsigned k=0; k < factor; k ++) {
				for (unsigned l=0; l < factor; l++) {
					rgb pix1 = render_pixel(&srcs[k][l], local_inf, &pal);
					rr += pix1.r; gg += pix1.g; bb += pix1.b;
				}
			}
			switch(fmt) {
				case CAIRO_FORMAT_ARGB32:
				case CAIRO_FORMAT_RGB24:
					// alpha=1.0 so these cases are the same.
					dst[3] = 0xff;
					dst[2] = rr/(factor * factor);
					dst[1] = gg/(factor * factor);
					dst[0] = bb/(factor * factor);
					dst += 4;
					break;

				case pixpack_format::PACKED_RGB_24:
					dst[0] = rr/(factor * factor);
					dst[1] = gg/(factor * factor);
					dst[2] = bb/(factor * factor);
					dst += 3;
					break;

				default:
					abort();
			}
			for (unsigned k=0; k < factor; k++) {
				srcs[k] += factor;
			}
		}
	}
	return true;
}
