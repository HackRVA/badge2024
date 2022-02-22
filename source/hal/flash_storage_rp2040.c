//
// Created by Samuel Jones on 11/6/21.
//

#include <string.h>

#include "hardware/flash.h"
#include "pico/multicore.h"

#include "flash_storage.h"

// Cache area here. For now, mostly used to enable random write.
// Right now this is only used with interrupts disabled and the other core locked out, so no
// extra locking is needed
static uint8_t _flash_cache[FLASH_PAGE_SIZE];

size_t flash_data_read(uint8_t sector, uint16_t offset, uint8_t *buf, size_t len) {

    // Ensure we want to write a valid location
    size_t address = ((STORAGE_BASE) + sector * FLASH_SECTOR_SIZE + offset);
    if (address >= NOR_FLASH_SIZE) {
        return 0;
    }

    // Limit read to the end of the NOR flash
    len = MIN(len, NOR_FLASH_SIZE - address);

    // Read data
    memcpy(buf, (const uint8_t*)(XIP_BASE + address), len);

    return len;

}

size_t flash_data_write(uint8_t sector, uint16_t offset, const uint8_t *buf, size_t len) {

    // Ensure we want to write a valid location
    size_t address = ((STORAGE_BASE) + sector * FLASH_SECTOR_SIZE + offset);
    if (address >= NOR_FLASH_SIZE) {
        return 0;
    }

    // Limit write to the end of the NOR flash
    len = MIN(len, NOR_FLASH_SIZE - address);

    // Figure out which pages we will be iterating over.
    uint32_t start_page_address = address & (~(FLASH_PAGE_SIZE-1));
    uint32_t last_page_address = (address + len - 1) & (~(FLASH_PAGE_SIZE-1));

    // Prevent interrupts and we must prevent the other core from XIP access.
    // The simplest way to do this is to make it pause.
    assert(multicore_lockout_start_timeout_us(1000*1000) == true); // Blocking lockouts causing assertions in the SDK
    uint32_t interrupt_status = save_and_disable_interrupts();

    // Use the cache area to enable random write.
    for (uint32_t i=start_page_address; i<=last_page_address; i+=FLASH_PAGE_SIZE) {

        // Fill area in page before data, if there is any. Writing 0xFF allows the
        // area to be written again later.
        uint32_t page_offset = 0;
        if (i < address) {
            page_offset = address-i;
            memset(&_flash_cache[0], 0xFF, page_offset);
        }

        // Fill data that goes in this page.
        uint32_t page_data_len = MIN(FLASH_PAGE_SIZE-page_offset, len);
        if (page_data_len) {
            memcpy(&_flash_cache[page_offset], buf, page_data_len);
            buf += page_data_len;
            len -= page_data_len;
        }

        // Fill dummy data (0xFF) at the end fo the page, if there is any.
        uint32_t empty_page_len = FLASH_PAGE_SIZE - (page_data_len + page_offset);
        if (empty_page_len >= 0) {
            memset(&_flash_cache[FLASH_PAGE_SIZE-empty_page_len], 0xFF, empty_page_len);
        }

        // Do the write.
        flash_range_program((uint32_t)i, _flash_cache, FLASH_PAGE_SIZE);
    }

    // We're done, so allow the other core and interrupts to run now.
    restore_interrupts(interrupt_status);
    assert(multicore_lockout_end_timeout_us(1000*1000) == true);

    return len;
}

void flash_erase(uint8_t sector) {

    // Prevent interrupts and we must prevent the other core from XIP access.
    // The simplest way to do this is to make it pause.
    assert(multicore_lockout_start_timeout_us(1000*1000) == true);
    uint32_t interrupt_status = save_and_disable_interrupts();

    flash_range_erase((STORAGE_BASE) + sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);

    restore_interrupts(interrupt_status);
    assert(multicore_lockout_end_timeout_us(1000*1000) == true);
}

void flash_erase_all(void) {
    for (uint8_t i=0; i<NUM_DATA_SECTORS; i++) {
        flash_erase(i);
    }
}