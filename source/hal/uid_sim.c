/*!
 *  @file   uid_rp2040.c
 *  @author Peter Maxwell Warasila
 *  @date   June 6, 2022
 *
 *  @brief  RVASec 2023 Badge Unique ID RP2040 implementation
 *
 *------------------------------------------------------------------------------
 *
 */

#include <stdint.h>

#ifndef BADGE_ID_SIM
#define BADGE_ID_SIM (123456789ull)
#endif

uint64_t uid_get()
{
    return BADGE_ID_SIM;
}
