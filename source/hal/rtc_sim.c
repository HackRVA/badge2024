//
// Created by Samuel Jones on 2/21/22.
//

#include "rtc.h"
#include <time.h>
#include <sys/time.h>

uint64_t boot_microseconds = 0;

void rtc_init_badge(__attribute__((unused)) time_t start_time) {

    struct timespec now;
    timespec_get(&now, TIME_UTC);
    boot_microseconds = now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

void rtc_set_time(__attribute__((unused)) time_t start_time) {
    // Just use system time, do nothing here
}

time_t rtc_get_unix_seconds(void) {
    return time(NULL);
}

struct timeval rtc_get_time_of_day(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv;
}

uint64_t rtc_get_ms_since_boot(void) {
    return rtc_get_us_since_boot()/1000;
}

uint64_t rtc_get_us_since_boot(void) {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    uint64_t now_microseconds = now.tv_sec * 1000000 + now.tv_nsec / 1000;
    return now_microseconds;
}
