/**
    simple assetList library
    Author: Paul Bruggeman
    paul@killercats.com
    4/2015
*/

#include "assetList.h"
#include "assets.h"

//#define HACKRVA4BIT_LORES
#include "hackrva4bit.h"

#define DRBOB_HIRES
#include "drbob.h"

#define RVASEC2016BITS_LORES
#include "rvasec2016.h"

//#include "font_2.xbm" // for size reasons just A-Z and a couple others
#include "font8x8.xbm.h" // full font


/* for 1 bit images */
/* testing color instead of BW */
const static char BW_cmap[2][3] = {
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
const struct asset assetList[] = {
        { DRBOB, DRBOB_BITS, 1, DRBOB_WIDTH, DRBOB_HEIGHT, (const char *)DRBOB_CMAP, (const char *)DRBOB_DATA, (drawLCD8) },
        { HACKRVA4, HACKRVA4_BITS, 1, HACKRVA4_WIDTH, HACKRVA4_HEIGHT, (const char *)HACKRVA4_CMAP, (const char *)HACKRVA4_DATA, (drawLCD4) },
// partial font    { FONT, PICTURE1BIT, 42, 8, 8, (const char *)BW_cmap, (const char *)font_2_bits, (drawLCD1) },
        { RVASEC2016, RVASEC2016_BITS, 1, RVASEC2016_WIDTH, RVASEC2016_HEIGHT, (const char *)RVASEC2016_CMAP, (const char *)RVASEC2016_DATA, (drawLCD2) },
        { FONT, PICTURE1BIT, 128, 8, 8, (const char *)BW_cmap, (const char *)font8x8_bits, (drawLCD1) },
};

