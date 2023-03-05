#include <stdint.h>

/* From: https://groups.google.com/g/comp.lang.c/c/IpwKbw0MAxw/m/N1xhe5n1rFoJ */

/*
 * long sqrtL2L( long X );
 *
 * Long to long point square roots.
 * RETURNS the integer square root of X (long).
 * REMAINDER is in the local variable r of type long on return.
 * REQUIRES X is positive.
 *
 * Christophe MEESSEN, 1993.
 */

/*
 * I only changed the name to fxp_sqrt and used int32_t instead of long) -- steve
 *
 * The way you use this thing is like:
 *
 * int32_t answer = fxp_sqrt(int32_value << 8) >> 8;
 */
int32_t fxp_sqrt(int32_t x)
{
	uint32_t t, q, b, r;

	r = x;
	b = 0x40000000;
	q = 0;

	while( b >= 256 ) {
		t = q + b;
		q = q / 2;     /* shift right 1 bit */
		if( r >= t ) {
			r = r - t;
			q = q + b;
		}
		b = b / 4;     /* shift right 2 bits */
	}
	return q;
}
