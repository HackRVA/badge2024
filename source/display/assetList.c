/**
    simple assetList library
    Author: Paul Bruggeman
    paul@killercats.com
    4/2015
*/

#include "assetList.h"

#define DRBOB_HIRES
#include "drbob.h"

//#include "font_2.xbm" // for size reasons just A-Z and a couple others
#include "font8x8.xbm.h" // full font
#include "font8x8-rotated.xbm.h" // rotated 90 deg clockwise font


/* for 1 bit images */
/* testing color instead of BW */
static const char BW_cmap[2][3] = {
        { 32, 32, 32 },
        { 64, 192, 64 },
};

/*
   all pixels should fill a whole byte and be evenly divided.
   1 bit/pixel image has a min width of 8 pixels
   2 bits/pixel = 4 pixels wide min.
   4 bits/pixel = 2 pixels wide min.

 NOTE
 NOTE   if you add or remove an item ALSO add or remove it in the enum in assetList.h
 NOTE

*/

/* All of the asset drawing functions are now handled through framebuffer.h */
void dummy_draw(unsigned char aid, int frame)
{
        (void) aid;
        (void) frame;
        return;
}

const struct asset assetList[] = {
        { DRBOB, DRBOB_BITS, 1, DRBOB_WIDTH, DRBOB_HEIGHT, (const char *)DRBOB_CMAP, (const char *)DRBOB_DATA, (dummy_draw) },
// partial font    { FONT, PICTURE1BIT, 42, 8, 8, (const char *)BW_cmap, (const char *)font_2_bits, (dummy_draw) },
        { FONT, PICTURE1BIT, 128, 8, 8, (const char *)BW_cmap, (const char *)font8x8_bits, (dummy_draw) },
	{ ROTATED_FONT, PICTURE1BIT, 128, 8, 8, (const char *)BW_cmap, (const char *)font8x8_rotated_bits, (dummy_draw) },
};

