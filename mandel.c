#include <gtk/gtk.h>
#include <stdlib.h>
#include "mandel.h"

void draw_set(GdkPixmap * dest, GdkGC *gc, gint xx, gint yy, gint width, gint height)
{
	const gint rowbytes = width * 3;
	const gint rowstride = rowbytes + 8-(rowbytes%8);
	guchar *buf = malloc(rowstride * height); // packed 24-bit data

	int x,y;
	for (y=0; y<height; y++) {
		const guchar B = y*255/width;
		for (x=0; x<width; x++) {
			buf[y*rowstride + 3*x] = x*255/width;
			buf[y*rowstride + 3*x+1] = 128;
			buf[y*rowstride + 3*x+2] = B;
		}
	}

	gdk_draw_rgb_image(dest, gc,
			xx, yy, width, height, GDK_RGB_DITHER_NONE, buf, rowstride);

	free(buf);
}
