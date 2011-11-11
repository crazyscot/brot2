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

#include <png.h>
#include <stdlib.h>
#include "Render.h"
#include "Plot2.h"
#include "Exception.h"

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
bool Render::render_generic(unsigned char *buf, const int rowstride, const int local_inf, pixpack_format fmt, Plot2& plot, unsigned rwidth, unsigned rheight, unsigned /*antialias*/factor, const BasePalette& pal)
{
	ASSERT(buf);
	ASSERT((unsigned)rowstride >= RGB_BYTES_PER_PIXEL * rwidth);

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
					THROW(Exception,"Unhandled pixpack format "+(int)fmt);
			}
			for (unsigned k=0; k < factor; k++) {
				srcs[k] += factor;
			}
		}
	}
	delete[] srcs;
	return true;
}

// returns 0 for success, !0 for failure
int Render::save_as_png(FILE *f, const unsigned width, const unsigned height,
		Plot2& plot, const BasePalette& pal, const int antialias,
		const char** error_string_o)
{
	*error_string_o = 0;
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop png_info=0;
	if (png)
		png_info = png_create_info_struct(png);
	if (!png_info) {
		*error_string_o = "Could not create PNG structs (out of memory?)";
		if (png)
			png_destroy_write_struct(&png, 0);
		return ENOMEM;
	}

#if 0
	jmp_buf jbuf;
	if (setjmp(jbuf)) {
		fclose(f);
		return;
	}
#endif

	png_init_io(png, f);

	png_set_compression_level(png, Z_BEST_SPEED);
	png_set_IHDR(png, png_info,
			width, height, 8 /* 24bpp */,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	std::string comment = plot.info(true);
	const char* SOFTWARE = "brot2",
		      * INFO = comment.c_str();
	png_text texts[2] = {
			{PNG_TEXT_COMPRESSION_NONE, (char*)"Software", (char*)SOFTWARE, strlen(SOFTWARE)},
			{PNG_TEXT_COMPRESSION_NONE, (char*)"Comment", (char*)INFO, strlen(INFO) },
	};
	png_set_text(png, png_info, texts, 2);

	png_write_info(png, png_info);

	const int rowstride = Render::RGB_BYTES_PER_PIXEL * width;

	unsigned char * pngbuf = new unsigned char[rowstride * height];
	Render::render_generic(pngbuf, rowstride, -1, Render::pixpack_format::PACKED_RGB_24, plot, width, height, antialias, pal);
	unsigned char ** pngrows = new unsigned char*[height];
	for (unsigned i=0; i<height; i++) {
		pngrows[i] = pngbuf + rowstride*i;
	}
	png_write_image(png, pngrows);
	delete[] pngrows;
	delete[] pngbuf;
	png_write_end(png,png_info);
	png_destroy_write_struct(&png, &png_info);

	return 0;
}

/* Returns data for a single fractal point, identified by its pixel co-ordinates within a plot. */
const Fractal::PointData& Render::single_pixel_data(Plot2& plot,
		int x, int y, unsigned antialias_factor)
{
	// de-antialias, argh
	int xx=x*antialias_factor, yy=y*antialias_factor;
	return plot.get_pixel_point(xx,yy);
}
