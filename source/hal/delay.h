//
// Created by Samuel Jones on 11/10/21.
//

#include <stdint.h>

#ifndef badge2022_c_DELAY_H
#define badge2022_c_DELAY_H

void sleep_ms(uint32_t time);

void sleep_us(uint64_t time);

// Wrapper to disable/re-enable clocks when not being used.
void lp_sleep_us(uint64_t time);

#endif //badge2022_c_DELAY_H
