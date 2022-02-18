//
// Created by Samuel Jones on 2/11/22.
//

#include <time.h>

#ifndef BADGE2022_C_TIME_H
#define BADGE2022_C_TIME_H

void rtc_init_badge(time_t start_time);

void rtc_set_time(time_t start_time);

time_t rtc_get_unix_seconds(void);

struct timeval rtc_get_time_of_day(void);

uint64_t rtc_get_ms_since_boot(void);
uint64_t rtc_get_us_since_boot(void);


#endif //BADGE2022_C_TIME_H
