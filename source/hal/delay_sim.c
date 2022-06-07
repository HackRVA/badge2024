//
// Created by Samuel Jones on 2/21/22.
//

#include "delay.h"
#include <unistd.h>

void sleep_ms(uint32_t time) {
    usleep(time*1000);
}

void sleep_us(uint64_t time) {
    usleep(time);
}

void lp_sleep_us(uint64_t time) {
    usleep(time);
}