#ifndef PNG_UTILS_H__
#define PNG_UTILS_H__
/*
	Author: Stephen M. Cameron
*/
#include <png.h>

int png_utils_write_png_image(const char *filename, unsigned char *pixels, int w, int h, int has_alpha, int invert);

char *png_utils_read_png_image(const char *filename, int flipVertical, int flipHorizontal,
        int pre_multiply_alpha,
        int *w, int *h, int *hasAlpha, char *whynot, int whynotlen);

#endif
