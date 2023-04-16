/**
 *  @file   accelerometer.h
 *  @author Peter Maxwell Warasila
 *  @date   April 13, 2023
 *
 *  @brief  RVASec Badge accelerometer driver
 */

#ifndef BADGE_C_ACCELEROMTER_H
#define BADGE_C_ACCELEROMTER_H

#include <stdint.h>

#include <utils.h>

#ifdef ACCLEROMETER_FLOATING_POINT
#include <math.h>
typedef float mG_t;
#else
typedef int32_t mG_t;
#include <trig.h>
#include <fxp_sqrt.h>
#endif

enum acceleration_direction {
    ACCLERATION_X = 0,
    ACCLERATION_Y = 1,
    ACCLERATION_Z = 2,
};

union acceleration {
    mG_t a[3];
    struct {
        mG_t x;  /** Positive X on the badge faces down towards the USB port */
        mG_t y;  /** Positive Y on the badge faces right towards the hole */
        mG_t z;  /** Positive Z on the badge faces out towards the user */
    };
};

static inline mG_t acceleration_magnitude(union acceleration a)
{
    mG_t sum = 0;
    for (size_t i = 0; i < ARRAY_SIZE(a.a); i++) {
        sum += a.a[i];
    }

#ifdef ACCELEROMETER_FLOATING_POINT
    return sqrt(sum);
#else
    return fxp_sqrt(sum << 8) >> 8;
#endif
}

void accelerometer_init_gpio();
void accelerometer_init();
uint8_t accelerometer_whoami();
union acceleration accelerometer_last_sample();

#endif /* BADGE_C_ACCELEROMTER_H */
