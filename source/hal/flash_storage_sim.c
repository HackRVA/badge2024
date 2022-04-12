//
// Created by Samuel Jones on 2/21/22.
//

#include "flash_storage.h"
#include "flash_storage_config.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

const char *flash_filename = "simulator_flash_storage.bin";
static unsigned char flash_data[NUM_DATA_SECTORS][FLASH_SECTOR_SIZE];
static bool flash_loaded = false;

// flash is saved to file on termination to avoid unnecessary flushing to file.
void save_flash(void) {

    FILE *f = fopen(flash_filename, "wb");
    if (!f) {
        printf("error opening flash file to save: %s\n", flash_filename);
        return;
    }
    fwrite(flash_data, 1, sizeof(flash_data), f);
    fclose(f);
    printf("Saved flash data at %s\n", flash_filename);
}

static void load_flash(void) {

    memset(flash_data, 0xFF, sizeof(flash_data));
    FILE *f = fopen(flash_filename, "rb");
    if (!f) {
        return;
    }
    size_t loaded = fread(flash_data, 1, sizeof(flash_data), f);
    fclose(f);

    printf("Loaded %zu bytes of flash data from %s\n", loaded, flash_filename);
}

size_t flash_data_read(uint8_t sector, uint16_t offset, uint8_t *buf, size_t len) {
    if (!flash_loaded) {
        load_flash();
        flash_loaded = true;
    }

    int max_len = (NUM_DATA_SECTORS - sector) * FLASH_SECTOR_SIZE - offset;
    if (max_len < 0) {
        // invalid offset
        return 0;
    }
    if (max_len <= (int)len) {
        len = max_len;
    }

    memcpy(buf, &flash_data[sector][offset], len);
    return len;
}

size_t flash_data_write(uint8_t sector, uint16_t offset, const uint8_t *buf, size_t len) {
    if (!flash_loaded) {
        load_flash();
        flash_loaded = true;
    }

    int max_len = (NUM_DATA_SECTORS - sector) * FLASH_SECTOR_SIZE - offset;
    if (max_len < 0) {
        // invalid offset
        return 0;
    }
    if (max_len <= (int)len) {
        len = max_len;
    }

    for (size_t i=0; i<len; i++) {
        // NOR flash writes pull 1s to 0s only.
        flash_data[sector][offset+i] &= buf[i];
    }

    return len;
}

void flash_erase(uint8_t page) {
    if (!flash_loaded) {
        load_flash();
        flash_loaded = true;
    }
    if (page < NUM_DATA_SECTORS) {
        memset(flash_data[page], 0xFF, FLASH_SECTOR_SIZE);
    }
}

void flash_erase_all(void) {
    if (!flash_loaded) {
        load_flash();
        flash_loaded = true;
    }

    memset(flash_data, 0xFF, sizeof(flash_data));
}

void flash_deinit(void) {
    save_flash();
}