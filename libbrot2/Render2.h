/*
    Render2.h: Generic rendering functions
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

#ifndef RENDER2_H_
#define RENDER2_H_

#include <list>
#include <cairo/cairo.h>
#include "Fractal.h"
#include "palette.h"
#include "Plot3Chunk.h"

namespace Render2 {

const int RGB_BYTES_PER_PIXEL = 3;

class pixpack_format {
	// A pixel format identifier that supersets Cairo's.
	int f; // Pixel format - one of cairo_format_t or our internal constants
public:
	static const int PACKED_RGB_24 = CAIRO_FORMAT_RGB16_565 + 1000000;
	pixpack_format(int c): f(c) {};
	inline operator int() const { return f; }
};

// Renders a single pixel, given the current idea of infinity and the palette to use.
inline rgb render_pixel(const Fractal::PointData data, const int local_inf, const BasePalette * pal) {
	if (data.iter == local_inf || data.iter<0) {
		return black; // from Palette
	} else {
		return pal->get(data);
	}
}


class Generic {
	unsigned char *_buf;
	const unsigned _rowstride, _width, _height, _local_inf;
	const pixpack_format _fmt;
	const BasePalette& _pal;
	unsigned _pixelstep; // effectively const

public:
	/*
	 * buf: Where to put the data. This should be at least
	 * (rowstride * height) bytes long.
	 *
	 * rowstride: the size of an output row, in bytes. In other words the byte
	 * offset from one row to the next - which may be different from
	 * (bytes per pixel * width) if any padding is required.
	 *
	 * local_inf: the local plot's current idea of infinity.
	 * (N.B. -1 is always treated as infinity.)
	 *
	 * fmt: The byte format to use. This may be a CAIRO_FORMAT_* or our
	 * internal PACKED_RGB_24 (used for png output).
	 */
	Generic(unsigned char *buf, const int rowstride, unsigned width, unsigned height,
			const int local_inf, pixpack_format fmt, const BasePalette& pal);

	// TODO ANTIALIAS

	virtual ~Generic();

	void process(const Plot3Chunk& chunk);
	void process(const std::list<Plot3Chunk*>& chunks);



#if 0
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
	static bool render_generic(unsigned char *buf, const int rowstride, const int local_inf, pixpack_format fmt, Plot2& plot, unsigned rwidth, unsigned rheight, unsigned /*antialias*/factor, const BasePalette& pal);

	/* Returns data for a single fractal point, identified by its pixel co-ordinates within a plot. */
	static const Fractal::PointData& single_pixel_data(Plot2& plot, int x, int y, unsigned antialias_factor);

	// Renders a single pixel, given the current idea of infinity and the palette to use.
	static inline rgb render_pixel(const Fractal::PointData *data, const int local_inf, const BasePalette * pal) {
		if (data->iter == local_inf || data->iter<0) {
			return black; // from Palette
		} else {
			return pal->get(*data);
		}
	}

	// Renders a plot as a PNG.
	// FILE* f must be open in mode wb and ready to write.
	// width and height are in pixels.
	// plot and pal are the plot to render and palette to use.
	// antialias is the antialias factor we're using (1 in most cases).
	// Return: 0 for success, 1 for failure (in which case *error_string_o
	// is updated to point to a description of the error).
	static int save_as_png(FILE *f, const unsigned width, const unsigned height,
			Plot2& plot, const BasePalette& pal, const int antialias,
			const char** error_string_o);
#endif

};

}; // namespace Render2

#endif /* RENDER2_H_ */
