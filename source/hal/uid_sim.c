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

static uint64_t badge_id = BADGE_ID_SIM;

uint64_t uid_get(void)
{
    return badge_id;
}

void set_custom_badge_id(uint64_t id)
{
	badge_id = id;
}
