//
// Created by Samuel Jones on 2/21/22.
//

#include "flash_storage.h"
#include <stdio.h>

size_t flash_data_read(uint8_t page, uint16_t offset, uint8_t *buf, size_t len) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return 0;
}

size_t flash_data_write(uint8_t page, uint16_t offset, const uint8_t *buf, size_t len) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return 0;
}

void flash_erase(uint8_t page) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}

void flash_erase_all(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}