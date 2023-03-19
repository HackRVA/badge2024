#ifndef __BLINE_H
#define __BLINE_H

/* Plotting function is called by bline to plot points (or do something with them)
 * context is a cookie passed through bline to plotting_function.
 * If plotting function returns non zero, bline will stop early.
 */
typedef int (*plotting_function)(int x, int y, void *context);

extern void bline(int x1, int y1, int x2, int y2, plotting_function plot_func, void *context);

#endif
