/**
 *  @brief  C Utility Macros and Functions
 *  @author Peter Maxwell Warasila
 *  @date   March 19, 2023
 *
 *  SPDX-License-Identifier: MIT
 *
 *  
 *  Peter's Handy C Utility Macros and Functions
 *  --------------------------------------------
 *      "I wrote them so you don't have to!"
 *                                     - PMW
 *
 *  This is a collection of common C utility macros and functions which show up
 *  with enough regularity to warrant never having to write them again. Feel
 *  free to include this in any of your badge applications or drivers.
 */

#ifndef BADGE_UTILS_H
#define BADGE_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*- Macro Utilities ----------------------------------------------------------*/
/**
 *  @brief  A macro with an empty expression
 * 
 *  This can be useful at times.
 */
#define EMPTY

/**
 *  @brief  A macro that expands to its argument.
 *  
 *  This can be useful in cases where the preprocessor needs to expand an extra
 *  time before using @p a
 */
#define IDENTITY(a) a

/*- Bits and Bitfields -------------------------------------------------------*/
/**
 *  @brief  Unsigned integer with a bit in position @p n set.
 */
#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

/**
 *  @brief  Set or clear bit at @p pos
 * 
 *  @param  a   Variable to alter
 *  @param  pos Bit position in @p a
 *  @param  set Whether to set or clear the bit at @p pos in @p a
*/
#define WRITE_BIT(a, pos, set) \
    ((a) = (set) ? ((a) | BIT(pos)) : ((a) & ~BIT(pos)))

/*- Array Utilities ----------------------------------------------------------*/
/**
 *  @brief  Returns the number of elements in the provided @p array
 *  
 *  @param array array to size
 */
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof((array[0])))

/**
 *  @brief  Whether or not @p ptr is within @p array
 *  
 *  This macro evalutes that:
 *      1. The pointer is not null.
 *      2. The pointer is within the address range of the array.
 *  
 *  @param  array   given array
 *  @param  ptr     pointer to evaluate 
 */
#define PART_OF_ARRAY(array, ptr) \
    ((NULL != ptr) \
    && (((uintptr_t) array) <= ((uintptr_t) ptr)) \
    && (((uintptr_t) ptr) < ((uintptr_t) &(array)[ARRAY_SIZE(array)])))

/**
 *  @brief  Whether or not @p ptr is an element of @p array
 *  
 *  This macro evalutes that:
 *      1. The pointer is not null.
 *      2. The pointer is within the address range of the array.
 *      3. The pointer is aligned to an element of the array.
 *  
 *  @param  array   given array
 *  @param  ptr     pointer to evaluate 
 */
#define IS_ARRAY_ELEMENT(array, ptr) \
    (PART_OF_ARRAY(array, ptr) \
    && ((((uintptr_t) ptr) - ((uintptr_t) array)) % sizeof((array)[0]) == 0))

/*- Structure Tools ----------------------------------------------------------*/
/**
 *  @brief  Get a pointer to the structure containing the element
 * 
 *  This is an extremely useful macro for situations where it is warranted. It
 *  also enables some useful abstraction.
 *
 *  @param  ptr     Pointer to the structure element
 *  @param  type    Type which @p ptr is an element of
 *  @param  field   Field in @p type which @p ptr is
 */
#define CONTAINER_OF(ptr, type, field) \
    ((type *)((void *)(ptr)) - offsetof(type, field))

/*- Value Utilities ----------------------------------------------------------*/
#ifndef MAX
/**
 *  @brief  Returns the maximum of two values
 *
 *  @param  a   First value
 *  @param  b   Second value
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
/**
 *  @brief  Returns the minimum of two values
 * 
 *  @param  a   First value
 *  @param  b   Second value
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef CLAMP
/**
 *  @brief  Clamps the value within the given bounds (inclusive)
 *
 *  @param  a       Value to guard
 *  @param  lower   Lower bound (inclusive)
 *  @param  upper   Upper bound (inclusive)
 */
#define CLAMP(a, lower, upper) (((a) <= (lower)) ? (lower) : MIN(a, upper))
#endif

/*- Miscellaneous ------------------------------------------------------------*/
/**
 *  @brief  Number of sized bins to hold given items
 *
 *  @param  items       Number of items to hold
 *  @param  bin_size    Number of items each bin can hold
 */
static inline size_t minimum_bins(size_t items, size_t bin_size)
{
    return (items / bin_size) + ((items % bin_size != 0) ? 1 : 0);
}

#endif /* BADGE_UTILS_H */