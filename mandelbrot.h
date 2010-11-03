/*
    mandelbrot.h: Mandelbrot set interface for brot2
    Copyright (C) 2010 Ross Younger

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

#ifndef MANDELBROT_H_
#define MANDELBROT_H_

/* Ordinary Mandelbrot set */

typedef struct {
	double re, im;
} complexF;

typedef struct mandelbrot_ctx {
	complexF centre;
	complexF size;

	int priv_x; // Internal use only
	int priv_y; // Internal use only
} mandelbrot_ctx;

// TODO: Make engine a C++ class
// TODO: Variable parameters per-engine?

mandelbrot_ctx * mandelbrot_new(void);
void mandelbrot_destroy(mandelbrot_ctx **ctx);

/* Textual information about this set.
 * The result is ALLOCATED; caller must free when finished. */
const char * mandelbrot_info(mandelbrot_ctx *ctx);

/*
 * Draws this set to the given pixmap using the given GC.
 * x and y give the pixel origin. (TODO: remove us, assume 0?)
 * height and width are in pixels.
 */
void mandelbrot_draw(mandelbrot_ctx *ctx,
		GdkPixmap * dest, GdkGC *gc, gint x, gint y, gint width, gint height);

/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
 * Returns 1 for success, 0 if the point was outside of the render */
int mandelbrot_pixel_to_set(mandelbrot_ctx *ctx, gint x, gint y, complexF * set);


#endif
