//
// Created by Samuel Jones on 11/6/21.
//

#include <stdint.h>
#include <stddef.h>

#include "flash_storage_config.h"

#ifndef badge2022_c_FLASH_STORAGE_H
#define badge2022_c_FLASH_STORAGE_H

/** @brief Read data from the flash storage region into RAM.
 *
 * @param page - Input page number, valid range 0 to FLASH_SECTOR_SIZE-1
 * @param offset - Offset from the start of the page to read. Offsets larger than FLASH_SECTOR_SIZE are valid but will
 *                 read other pages.
 * @param buf - Buffer to put the data into.
 * @param len - Number of bytes to read into the output.
 * @return Number of bytes that were read into the output.
 */
size_t flash_data_read(uint8_t page, uint16_t offset, uint8_t *buf, size_t len);

/** @brief Write data to the flash storage region from RAM.
 *
 * Within the NOR flash this means that bits that are 1 will be changed to 0.
 *
 * @param page - Base page number to write to, valid range 0 to FLASH_SECTOR_SIZE-1
 * @param offset - Offset from the start of the page to write. Offsets larger than FLASH_SECTOR_SIZE are valid but will
 *                 write into other pages. Random write is allowed due to an internal buffer, though the SDK only
 *                 supports page writes.
 * @param buf - Data to write to the flash.
 * @param len - Length of data in bytes.
 * @return Number of bytes of data that were written.
 */
size_t flash_data_write(uint8_t page, uint16_t offset, const uint8_t *buf, size_t len);

/** @brief Erase the provided flash erase page number.
 *
 * This will reset all bits in the page to 1.
 **/
void flash_erase(uint8_t page);

/** @brief Wipe out the entire data region. */
void flash_erase_all(void);

/** @brief Finalize / commit any flash changes. */
void flash_deinit(void);

#endif //badge2022_c_FLASH_STORAGE_H
