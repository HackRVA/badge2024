//
// Created by Samuel Jones on 11/6/21.
//

#ifndef badge2022_c_FLASH_STORAGE_CONFIG_H
#define badge2022_c_FLASH_STORAGE_CONFIG_H


// Number of flash sectors reserved for data area,
#define NUM_DATA_SECTORS (8)

// FLASH_SECTOR_SIZE is 4096 and is defined in the Pico SDK
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE (4096)
#endif

// FLASH_PAGE_SIZE is 256 and is defined in the Pico SDK
#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE (256)
#endif

// NOR_FLASH_SIZE will need to change (and likely PICO_FLASH_SIZE_BYTES) if running pico code on the real target if we
// pick a different flash.
#ifdef PICO_FLASH_SIZE_BYTES
#define NOR_FLASH_SIZE (PICO_FLASH_SIZE_BYTES)
#else
#define NOR_FLASH_SIZE (2*1024*1024)
#endif

// Start of data storage area.
#define STORAGE_BASE (NOR_FLASH_SIZE - (NUM_DATA_SECTORS * FLASH_SECTOR_SIZE))


#endif //badge2022_c_FLASH_STORAGE_CONFIG_H
