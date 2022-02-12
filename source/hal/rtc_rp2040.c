//
// Created by Samuel Jones on 2/11/22.
//


#include "pico/time.h"
#include "hardware/rtc.h"
#include "rtc.h"
#include <stdio.h>

void rtc_init_badge(time_t start_time) {
    rtc_init();
    if (start_time != 0) {
        rtc_set_time(start_time);
    }
}

void rtc_set_time(time_t start_time) {
    struct tm time = *localtime(&start_time);
    datetime_t pico_time;
    pico_time.year = time.tm_year+1900;
    pico_time.month = time.tm_mon;
    pico_time.day = time.tm_mday;
    pico_time.hour = time.tm_hour;
    pico_time.min = time.tm_min;
    pico_time.sec = time.tm_sec;
    pico_time.dotw = time.tm_wday;
    rtc_set_datetime(&pico_time);
}

time_t rtc_get_unix_seconds(void) {
    datetime_t pico_time;
    rtc_get_datetime(&pico_time);

    struct tm time = {0};
    time.tm_year = pico_time.year-1900;
    time.tm_mon = pico_time.month;
    time.tm_mday = pico_time.day;
    time.tm_wday = pico_time.dotw;
    time.tm_hour = pico_time.hour;
    time.tm_min = pico_time.min;
    time.tm_sec = pico_time.sec;

    return mktime(&time);
}

struct timeval rtc_get_time_of_day(void) {
    datetime_t pico_time;
    rtc_get_datetime(&pico_time);

    struct timeval time = {0};
    time.tv_sec = pico_time.hour * 3600 + pico_time.min * 60 + pico_time.sec;
    return time;
}

uint64_t rtc_get_ms_since_boot(void) {
    return to_ms_since_boot(get_absolute_time());
}