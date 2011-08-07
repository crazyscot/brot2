/*
    render.h: Generic rendering functions
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

#ifndef RENDER_H_
#define RENDER_H_

#include <cairo/cairo.h>
#include "Fractal.h"
#include "palette.h"
#include "Plot2.h"

struct Render {

	class pixpack_format {
		// A pixel format identifier that supersets Cairo's.
		int f; // Pixel format - one of cairo_format_t or our internal constants
	public:
		static const int PACKED_RGB_24 = CAIRO_FORMAT_RGB16_565 + 1000000;
		pixpack_format(int c): f(c) {};
		inline operator int() const { return f; }
	};

/*
 * The actual work of turning an array of fractal_points - maybe an antialiased
 * set - into a packed array of pixels to a given format.
 *
 * buf: Where to put the data. This should be at least
 * (rowstride * rctx->height) bytes long.
 *
 * rowstride: the size of an output row, in bytes. In other words the byte
 * offset from one row to the next - which may be different from
 * (bytes per pixel * rctx->rwidth) if any padding is required.
 *
 * local_inf: the local plot's current idea of infinity.
 * (N.B. -1 is always treated as infinity.)
 *
 * fmt: The byte format to use. This may be a CAIRO_FORMAT_* or our
 * internal PACKED_RGB_24 (used for png output).
 *
 * plot: The plot to render.
 *
 * rwidth, rheight: The actual rendering width and height as a sanity check
 *
 * factor: The antialias factor to use (usually 1 or 2; simple linear antialias)
 *
 * pal: The palette to use.
 *
 * Returns: True if the render completed, false if the plot disappeared under
 * our feet (typically by the user doing something to cause us to render
 * afresh).
 */
	static bool render_generic(unsigned char *buf, const int rowstride, const int local_inf, pixpack_format fmt, Plot2& plot, unsigned rwidth, unsigned rheight, unsigned /*antialias*/factor, BasePalette& pal);

	// Renders a single pixel, given the current idea of infinity and the palette to use.
	static inline rgb render_pixel(const Fractal::PointData *data, const int local_inf, const BasePalette * pal) {
		if (data->iter == local_inf || data->iter<0) {
			return black; // from Palette
		} else {
			return pal->get(*data);
		}
	}

	static const int RGB_BYTES_PER_PIXEL = 3;
};

#endif /* RENDER_H_ */
