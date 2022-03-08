//
// Created by Samuel Jones on 2/11/22.
//

#include "random.h"
#include "hardware/structs/rosc.h"
#include <string.h>


void random_insecure_bytes(uint8_t *bytes, size_t len) {
    memset(bytes, 0, len);
    for (size_t i=0; i<len; i++) {
        for (int j=0; j<8; j++) {
            bytes[i] |= (rosc_hw->randombit << j);
        }
    }
}
