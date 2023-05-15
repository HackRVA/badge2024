#ifndef fb_h
#define fb_h

#include "assetList.h"

/*
   Frame buffer
*/
enum {
    FB_UNUSED=0,		/* unused */
    FB_CLEAR,			/* reset LCD */
    FB_BACKGROUND_COLOR,	/* 16 bit background fill color */
    FB_COLOR,			/* 16 bit color */
    FB_TRANSPARENT_MASK,	/* 16 bit transparent/key/mask color */
    FB_MOVE, 			/* (x 8bit, y 8bit) where the next stuff goes */
};

#define LCD_XSIZE 128
#define LCD_YSIZE 160

/* the output buffer */
#define FBSIZE (LCD_XSIZE * LCD_YSIZE)

struct vector8_t {
    unsigned char x;
    unsigned char y;
};

struct point
{
    signed char x, y;
};

struct framebuffer_t {
    unsigned short *buffer;
    struct vector8_t pos;
    unsigned char font;
    unsigned char rotated_font;
    unsigned char fontHeight;

    unsigned short color;
    unsigned short BGcolor;
    unsigned short transMask;
    unsigned short transIndex;
    unsigned short changed;
};

extern struct framebuffer_t G_Fb;

void FbInit(void) ;
void FbMove(unsigned char x, unsigned char y);
void FbMoveRelative(char x, char y);
void FbMoveX(unsigned char x);
void FbMoveY(unsigned char y);
void FbClear(void);
void FbColor(unsigned short color);
void FbBackgroundColor(unsigned short color);
// void FbPicture(unsigned char assetId, unsigned char seqNum);
void FbTransparency(unsigned short transparencyMask);
void FbTransparentIndex(unsigned short color);
// void FbSprite(unsigned char picId, unsigned char imageNo);
void FbCharacter(unsigned char charin);
void FbRotCharacter(unsigned char charin);
void FbFilledRectangle(unsigned char width, unsigned char height);
void FbPoint(unsigned char x, unsigned char y);
// void FbPrintChar(unsigned char charin, unsigned char x, unsigned char y);
void FbHorizontalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
void FbVerticalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
void FbLine(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1);

/* Draw a line clipped to the display.  At least 1 of (x0, y0), (x1, y1) must be on the display */
void FbClippedLine(signed short x0, signed short y0, signed short x1, signed short y1);

void FbWriteLine(const char *string);
void FbWriteString(const char *string);
void FbRotWriteLine(const char *string); /* write text rotated 90 degrees clockwise */
void FbRotWriteString(const char *string);
void FbRectangle(unsigned char width, unsigned char height);
void FbCircle(int x, int y, int r);

/** @brief Render the asset with its upper left corner at the current frame buffer location.
 *
 * Images can be generated using the tools/asset_converter.py script. */
void FbImage(const struct asset* asset, unsigned char seqNum);

/** @brief Render the provided raw 16-bit color asset. */
void FbImage16bit(const struct asset* asset, unsigned char seqNum);
/** @brief Render the provided 8-bit color asset, using its color palette table (and optional FbTransparentIndex). */
void FbImage8bit(const struct asset* asset, unsigned char seqNum);
/** @brief Render the provided 4-bit color asset, using its color palette table (and optional FbTransparentIndex). */
void FbImage4bit(const struct asset* asset, unsigned char seqNum);
/** @brief Render the provided 2-bit color asset, using its color palette table (and optional FbTransparentIndex). */
void FbImage2bit(const struct asset* asset, unsigned char seqNum);
/** @brief Render the provided 1-bit color asset, using the colors set by FbColor and FbBackgroundColor. */
void FbImage1bit(const struct asset* asset, unsigned char seqNum);

/* FbDrawObject() draws an object at x, y.  The coordinates of drawing[] should be centered at
 * (0, 0).  The coordinates in drawing[] are multiplied by scale, then divided by 1024 (via a shift)
 * so for 1:1 size, use scale of 1024.  Smaller values will scale the object down. This is different
 * than FbPolygonFromPoints() or FbDrawVectors() in that drawing[] contains signed chars, and the
 * polygons can be constructed via this program: https://github.com/smcameron/vectordraw
 * as well as allowing scaling.
 */
void FbDrawObject(const struct point drawing[], int npoints, int color, int x, int y, int scale);
void FbPushBuffer(void);
void FbSwapBuffers(void); // Currently does not swap buffers, just writes the current one.

#endif
