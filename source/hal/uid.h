/*!
 *  @file   uid.h
 *  @author Peter Maxwell Warasila
 *  @date   June 6, 2022
 *
 *  @brief  RVASec Badge Unique ID
 *
 *------------------------------------------------------------------------------
 *
 */

/*!
 *  Get unique ID from flash.
 *
 *  @return 64-bit unique ID from flash.
 */
uint64_t uid_get(void);

#if TARGET_SIMULATOR

void set_custom_badge_id(uint64_t id);

#endif
