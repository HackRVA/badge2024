//
// Created by Samuel Jones on 2/11/22.
//

#include <stdint.h>
#include <stddef.h>

#ifndef BADGE_C_RANDOM_H
#define BADGE_C_RANDOM_H

void random_insecure_bytes(uint8_t *bytes, size_t len);
uint32_t random_insecure_u32_congruence(uint32_t last);

#endif //BADGE_C_RANDOM_H
