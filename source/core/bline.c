#include "bline.h"
#include <stdlib.h>

/*
 * Bresenham's line drawing algorithm.
 */
void bline(int x0, int y0, int x1, int y1, plotting_function plot_func, void *context)
{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy)/2, e2;

	for(;;) {
		if (plot_func(x0, y0, context))
			return;

		if (x0==x1 && y0==y1)
			break;

		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

