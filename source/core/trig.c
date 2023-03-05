#include "trig.h"
#include <stdio.h>
#include <stdint.h>

/* 128 sine values * 256 */
static const int16_t sine_array[] = {
	0, 12, 25, 37, 49, 62, 74, 86, 97, 109, 120, 131, 142, 152, 162, 171, 181, 189, 197, 205, 212,
	219, 225, 231, 236, 241, 244, 248, 251, 253, 254, 255, 256, 255, 254, 253, 251, 248, 244, 241,
	236, 231, 225, 219, 212, 205, 197, 189, 181, 171, 162, 152, 142, 131, 120, 109, 97, 86, 74, 62,
	49, 37, 25, 12, 0, -12, -25, -37, -49, -62, -74, -86, -97, -109, -120, -131, -142, -152, -162,
	-171, -181, -189, -197, -205, -212, -219, -225, -231, -236, -241, -244, -248, -251, -253, -254,
	-255, -256, -255, -254, -253, -251, -248, -244, -241, -236, -231, -225, -219, -212, -205, -197,
	-189, -181, -171, -162, -152, -142, -131, -120, -109, -97, -86, -74, -62, -49, -37, -25, -12,
};

short sine(int a)
{
	return sine_array[a];
}

short cosine(int a)
{
	return sine_array[(a + 32) & 127];
}

/* Lookup table for angles 0 - 45 degrees (0 to 16 in our system).  x must be greater than or equal to y */
static int16_t atan_lookup_table(int16_t x, int16_t y)
{
	static unsigned char atan_lut[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 13, 14, 15, 15 };
	if (x == 0)
		return 0;
	/* x>=0, y>=0 and x>=y, so y/x will be in the range 0.0 to 1.0, so (16*y)/x will be in range 0 - 16 */
	return (int16_t) atan_lut[(16 * y) / x];
}

int16_t arctan2(int16_t y, int16_t x)
{
	int16_t angle, quadrant = 0;

	if (x < 0) {
		x = -x;
		quadrant = 1;
	}
	if (y < 0) {
		y = -y;
		quadrant |= 2;
	}
	if (x > y) /* angle is between 0 and 45 degrees */
		angle = atan_lookup_table(x, y);
	else /* angle is between 45 and 90 degrees */
		angle = 32 - atan_lookup_table(y, x);

	switch (quadrant) {
	case 0:
		return angle;
	case 1:
		return 64 - angle;
	case 2:
		return -angle;
	case 3:
	default:
		return angle - 64;
	}
}

