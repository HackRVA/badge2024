//
// Created by Samuel Jones on 2/21/22.
//

#include "random.h"
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

static bool seeded = false;

void random_insecure_bytes(uint8_t *bytes, size_t len) {

    if (!seeded) {
        srand((unsigned) time(NULL));
        seeded = true;
    }
    for (size_t i=0; i<len; i++) {
        bytes[i] = rand() & 0xFF;
    }
}