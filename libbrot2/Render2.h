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
#include <vector>
#include <cairo/cairo.h>
#include <png++/png.hpp>
#include <stdio.h>
#include "Fractal.h"
#include "palette.h"
#include "Plot3Chunk.h"

namespace Render2 {

using namespace Plot3;

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

inline rgb antialias_pixel(std::vector<rgb> const& pix) {
	unsigned R=0,G=0,B=0,npix=0;
	for (auto it = pix.cbegin(); it != pix.cend(); it++) {
		R += (*it).r;
		G += (*it).g;
		B += (*it).b;
		++npix;
	}
	return rgb(R/npix,G/npix,B/npix);
}

class Base {
public:
	Base(unsigned width, unsigned height, int local_inf, const BasePalette& pal);
	virtual ~Base() {}

	/** Processes a single chunk. */
	virtual void process(const Plot3Chunk& chunk) = 0;
	/** Processes a list of chunks. This usually leads to repeated calls to process(chunk). */
	virtual void process(const std::list<Plot3Chunk*>& chunks);
protected:
	unsigned _width, _height, _local_inf;
	const BasePalette& _pal;

	/**
	 * Non-antialiased chunk processing.
	 */
	void process_plain(const Plot3Chunk& chunk);
	/**
	 * Called by process_* functions for each output pixel.
	 * The X and Y parameters are relative to the output width/height.
	 */
	virtual void pixel_done(unsigned X, unsigned Y, const rgb& p) = 0;
};

class MemoryBuffer : public Base {
	/*
	 * Generic rendering to a memory buffer.
	 * Workflow:
	 * 1. Allocate your buffer, should be at least (rowstride * height) bytes long.
	 * 2. Instantiate this class
	 * 3. Process your chunks
	 * 4. When you're happy, do whatever is appropriate with the buffer.
	 */
	unsigned char *_buf;
	const unsigned _rowstride;
	const pixpack_format _fmt;
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
	MemoryBuffer(unsigned char *buf, const int rowstride, unsigned width, unsigned height,
			const int local_inf, pixpack_format fmt, const BasePalette& pal);

	// TODO ANTIALIAS

	virtual ~MemoryBuffer();

	using Base::process;
	virtual void process(const Plot3Chunk& chunk);

#if 0
	/* Returns data for a single fractal point, identified by its pixel co-ordinates within a plot. */
	static const Fractal::PointData& single_pixel_data(Plot2& plot, int x, int y, unsigned antialias_factor);
#endif

protected:
	virtual void pixel_done(unsigned X, unsigned Y, const rgb& p);
};

class PNG : public Base {
	/*
	 * Renders a plot as a PNG file.
	 * Workflow:
	 * 1. Determine your filename
	 * 2. Instantiate this class
	 * 3. Process your chunks
	 * 4. Call write() when you're ready to write the PNG.
	 *
	 * Note that this class contains a nontrivial memory buffer throughout its lifetime.
	 */
protected:
	bool _antialias;
	png::image< png::rgb_pixel > _png;

public:
	/*
	 * Width and height are in pixels.
	 *
	 * If antialias is true we apply a 2x downscale in either direction.
	 * Specify only the output height and width; we expect to process
	 * 4x as many pixels via the chunks system.
	 *
	 * CAUTION: Chunk widths and heights of antialiased plots must be even!
	 */
	PNG(unsigned width, unsigned height, const BasePalette& palette, int local_inf, bool antialias=false);
	virtual ~PNG();

	using Base::process;
	virtual void process(const Plot3Chunk& chunk);
	virtual void process_antialias(const Plot3Chunk& chunk);

	void write(const std::string& filename);
	void write(std::ostream& ostream);

protected:
	virtual void pixel_done(unsigned X, unsigned Y, const rgb& p);
};

}; // namespace Render2

#endif /* RENDER2_H_ */
