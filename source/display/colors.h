#ifndef colors_h
#define colors_h

#define RGBPACKED(R,G,B) ( ((unsigned short)(R)<<11) | ((unsigned short)(G)<<6) | (unsigned short)(B) )
#define PACKRGB(R,G,B) ( ((unsigned short)(R)<<11) | ((unsigned short)(G)<<6) | (unsigned short)(B) )
#define PACKRGB888(R, G, B)	(((((R) >> 3) & 0x1F) << 11) | ((((G) >> 2) & 0x3F) << 5) | (((B) >> 3) & 0x1F))

#define UNPACKR(PACKED) (((PACKED & 0b1111100000000000) >> 11) &  0b11111)
#define UNPACKG(PACKED) (((PACKED & 0b0000011111100000) >>  5) & 0b111111)
#define UNPACKB(PACKED)   (PACKED & 0b0000000000011111)

#define D_BLUE 0b0000000000000111
#define B_BLUE 0b1000010000011111
#define B_RED  0b1111000000100001

#define BLUE    0b0000000000011111
#define GREEN   0b0000011111100000
#define RED     0b1111100000000000

#define BLACK   0b0000000000000000
#define GREY1   0b0000100001000001
#define GREY2   0b0001000010000010
#define GREY4   0b0010000100000100
#define GREY8   0b0100001000001000
#define GREY16  0b1000010000010000
#define WHITE   0b1111111111111111

#define CYAN    0b0000011111111111
#define YELLOW  0b1111111111100000
#define MAGENTA 0b1111100000011111

#include "x11_colors.h"

#endif
