/*!
 *  @file   uid_rp2040.c
 *  @author Peter Maxwell Warasila
 *  @date   June 6, 2022
 *  
 *  @brief  RVASec 2022 Badge Unique ID RP2040 implementation
 *
 *------------------------------------------------------------------------------
 * 
 */

#include <stdint.h>
#include <string.h>

#include <pico/unique_id.h>

uint64_t uid_get()
{
    pico_unique_board_id_t id;
    uint64_t badgeId;
    pico_get_unique_board_id(&id);
    memcpy(&badgeId, &id, sizeof(id));
    return __builtin_bswap64(badgeId);
}
