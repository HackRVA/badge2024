#ifndef assetList_h
#define assetList_h

#include <stdint.h>

/*
 NOTE
 NOTE   LASTASSET has to be the LAST item in the enum
 NOTE   insert new enums before it.
 NOTE
*/
enum {
    DRBOB=0,
    HACKRVA4,
    RVASEC2016,
    RVASEC_LOGO,
    HOLLY01,
    HOLLY02,
    HOLLY03,
    FONT,
    ROTATED_FONT,

    LASTASSET,
};

enum {
    AUDIO,
    MIDI,
    PICTURE1BIT,
    PICTURE2BIT,
    PICTURE4BIT,
    PICTURE8BIT,
    PICTURE16BIT,
};

struct asset {
    unsigned char assetId; /* number used to reference object */
    unsigned char type;    /* image/audio/midi/private */
    unsigned char seqNum; /* number of images within the asset for animation, frame no. for font char id */
    unsigned short x;	/* array x */
    unsigned short y;	/* array y */
    const char *data_cmap; /* color map lookup table for image data */
    const char *pixdata;   /* color pixel data */
    void (*datacb)(unsigned char, int); /* routine that can display or play asset */
};

/* Similar to asset, but packs colors into 16 bits per color (as the display wants them) instead
 * of 24 bits per color.
 */
struct asset2 {
	unsigned char type;
	unsigned char seqNum;
	unsigned short x; /* width in pixels */
	unsigned short y; /* heightin pixels */
	const uint16_t  *colormap; /* colors used in image, 16 bits per color, 5 bits each for R, G, B */
	union {
		const unsigned char *pixel; /* uint8_t indices into colormap for each pixel in the image */
		const uint16_t *pixel16; /* directly stored 16 bit color data with no color map indexing */
	};
};

extern const struct asset assetList[];

#endif
