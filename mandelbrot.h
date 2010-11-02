#ifndef MANDELBROT_H_
#define MANDELBROT_H_

/* Ordinary Mandelbrot set */

typedef struct {
	double re, im;
} complexF;

typedef struct mandelbrot_ctx {
	complexF centre;
	complexF size;
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
 * x and y give the origin.
 * height and width are in pixels.
 */
void mandelbrot_draw(mandelbrot_ctx *ctx,
		GdkPixmap * dest, GdkGC *gc, gint x, gint y, gint width, gint height);



#endif
