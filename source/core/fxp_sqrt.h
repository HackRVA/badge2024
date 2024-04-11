#ifndef FXP_SQRT_H__
#define FXP_SQRT_H__

#include <stdint.h>

/* int32_t fxp_sqrt(int32_t x)
 *
 * Returns the approximate square root of x where x is a fixed point number
 * with 8 bits to the right of the fixed point.  The return value is also a fixed
 * point number with 8 bits to the right of the fixed point.
 *
 * Use like so:
 * int32_t answer = fxp_sqrt(number_you_want_sqrt_of << 8);
 *
 * The answer will be fixed point with 8 bits to the right of the fixed point
 * E.g. fxp_sqrt(100 < 8) yields (10 << 8)
 * Note fxp_sqrt(100) does not yield the correct answer (which would be 160, not 10),
 * For any value between 0 and 256 it yields 0.
 *
 */
int32_t fxp_sqrt(int32_t x);

/** Get integer square root for any u32.
 *
 *  @param  x   Square.
 *  
 *  @return Square root.
 */
uint32_t sqrtu32(uint32_t r);

#endif

