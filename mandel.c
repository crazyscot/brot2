#include <gtk/gtk.h>
#include <stdlib.h>
#include "mandel.h"

#define MAXITER 1000

typedef struct {
	double re, im;
} complexF;

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

void draw_set(GdkPixmap * dest, GdkGC *gc, gint xx, gint yy, gint width, gint height)
{
	init_coltab();

	const gint rowbytes = width * 3;
	const gint rowstride = rowbytes + 8-(rowbytes%8);
	guchar *buf = malloc(rowstride * height); // packed 24-bit data

	const complexF Mcentre = {-0.7, 0.0};
	const complexF Msize =  {3.076, 3.0};

	const double pixwide = Msize.re / width,
		  		 pixhigh  = Msize.im / height;
	const complexF Mstart = {
		Mcentre.re - Msize.re/2,
		Mcentre.im - Msize.im/2
	};


	int x,y;
	for (y=0; y<height; y++) {
		guchar *row = &buf[y*rowstride];
		double xpoint = Mstart.re, 
			   ypoint = Mstart.im + pixhigh * y;

		for (x=0; x<width; x++) {
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
