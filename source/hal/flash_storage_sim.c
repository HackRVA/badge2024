//
// Created by Samuel Jones on 2/21/22.
//

#include "flash_storage.h"
#include <stdio.h>

size_t flash_data_read(__attribute__((unused)) uint8_t page, __attribute__((unused)) uint16_t offset,
			__attribute__((unused)) uint8_t *buf, __attribute__((unused)) size_t len) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return 0;
}

size_t flash_data_write(__attribute__((unused)) uint8_t page, __attribute__((unused)) uint16_t offset,
			__attribute__((unused)) const uint8_t *buf, __attribute__((unused)) size_t len) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return 0;
}

void flash_erase(__attribute__((unused)) uint8_t page) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}

void flash_erase_all(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}
