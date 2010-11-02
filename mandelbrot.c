#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "mandelbrot.h"

#define MAXITER 1000

mandelbrot_ctx * mandelbrot_new(void)
{
	mandelbrot_ctx *rv = malloc(sizeof(mandelbrot_ctx));
	return rv;
}

void mandelbrot_destroy(mandelbrot_ctx **ctx)
{
	memset(*ctx, 0, sizeof(*ctx));
	free(*ctx);
	*ctx = 0;
}


struct {
	guchar r,g,b;
} coltab[MAXITER+1];

static int coltab_inited = 0;

static void init_coltab(void) {
	if (coltab_inited) return;
	int n;
	for (n=0; n<MAXITER; n++) {
		coltab[n].r = n*255/MAXITER;
		coltab[n].g = (MAXITER-n)*255/MAXITER;
		coltab[n].b = n*255/MAXITER;
	}
	coltab[MAXITER].r = coltab[MAXITER].g = coltab[MAXITER].b = 0;
	coltab_inited = 1;
}

const char *mandelbrot_info(mandelbrot_ctx *ctx)
{
	char *rv = 0;
	asprintf(&rv, "Centre re=%f im=%f; size re=%f im=%f",
			ctx->centre.re, ctx->centre.im,
			ctx->size.re, ctx->size.im);
	return rv;
}

void mandelbrot_draw(mandelbrot_ctx *ctx,
		GdkPixmap * dest, GdkGC *gc, gint xx, gint yy, gint width, gint height)
{
	ctx->priv_x = width;
	ctx->priv_y = height;

	init_coltab();

	const gint rowbytes = width * 3;
	const gint rowstride = rowbytes + 8-(rowbytes%8);
	guchar *buf = malloc(rowstride * height); // packed 24-bit data

	const double pixwide = ctx->size.re / width,
		  		 pixhigh  = ctx->size.im / height;
	const complexF Mstart = {
		ctx->centre.re - ctx->size.re/2,
		ctx->centre.im - ctx->size.im/2
	};

	int i,j;
	for (j=0; j<height; j++) {
		guchar *row = &buf[j*rowstride];
		double xpoint = Mstart.re,
			   ypoint = Mstart.im + pixhigh * j;

		for (i=0; i<width; i++) {
			unsigned iter = 0;
			double re=0, im=0;
			while ( ((re*re + im*im) <= 4.0) && iter < MAXITER) {
				double reprime = re * re - im * im + xpoint;
				double imprime = 2 * re * im + ypoint;
				re = reprime;
				im = imprime;
				++iter;
			}
			row[0] = coltab[iter].r;
			row[1] = coltab[iter].g;
			row[2] = coltab[iter].b;
			xpoint += pixwide;
			row += 3;
			//printf("%d,%d: re=%f, im=%f, iter=%d\n", x, y, xpoint,ypoint, iter);
			//printf("%d,%d: iter=%d\n", x, y, iter);
		}
	}

	gdk_draw_rgb_image(dest, gc,
			xx, yy, width, height, GDK_RGB_DITHER_NONE, buf, rowstride);

	free(buf);
}

/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates */
int mandelbrot_pixel_to_set(mandelbrot_ctx *ctx, gint x, gint y, complexF * set)
{
	if ((x < 0) || (y < 0) || (x > ctx->priv_x) || (y > ctx->priv_y))
		return 0;

	const complexF Mstart = {
		ctx->centre.re - ctx->size.re/2,
		ctx->centre.im - ctx->size.im/2
	};

	const double pixwide = ctx->size.re / ctx->priv_x,
		  		 pixhigh  = ctx->size.im / ctx->priv_y;

	set->re = Mstart.re + pixwide*x;
	set->im = Mstart.im + pixhigh*y;
	return 1;
}
