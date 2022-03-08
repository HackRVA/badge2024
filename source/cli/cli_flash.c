//
// Created by Samuel Jones on 11/9/21.
//

#include "cli_flash.h"
#include "flash_storage.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

int run_flash_write(char * args) {

    char * sector = cli_get_token(&args);
    if (!sector) {
        puts("No sector num provided.");
        return 1;
    }

    char * offset = cli_get_token(&args);
    if (!offset) {
        puts("No write offset provided.");
        return 1;
    }

    char * data = cli_get_token(&args);
    if (!data) {
        puts("No string data to write provided.");
        return 1;
    }

    int sector_num = strtol(sector, NULL, 10);
    int offset_num = strtol(offset, NULL, 10);
    int data_len = strlen(data);

    printf("Writing %u bytes of data at offset %u into data sector %u\n", data_len, offset_num, sector_num);
    size_t written = flash_data_write(sector_num, offset_num, (uint8_t*) data, data_len);
    printf(" ...wrote %u bytes of data\n", (unsigned int)written);
    return 0;
}

int run_flash_write_hex(char * args) {
    char * sector = cli_get_token(&args);
    if (!sector) {
        puts("No sector num provided.");
        return 1;
    }

    char * offset = cli_get_token(&args);
    if (!offset) {
        puts("No write offset provided.");
        return 1;
    }

    char * data = cli_get_token(&args);
    if (!data) {
        puts("No hex data to write provided.");
        return 1;
    }

    int sector_num = strtol(sector, NULL, 10);
    int offset_num = strtol(offset, NULL, 10);
    size_t data_len = strlen(data);
    if (data_len % 2 != 0) {
        puts("Only an even number of hex digits is supported.");
        return 1;
    }

    for (size_t i=0; i<data_len; i+=2) {
        uint8_t binary_data = 0;
        for (int j=0; j<2; j++) {
            // On second loop this will shift first nibble upward
            binary_data <<= 4;

            char nibble = data[i+j];
            // Convert lower to upper
            if (nibble >= 'a' && nibble <= 'f') {
                nibble -= 0x20;
            }

            if (nibble >= '0' && nibble <= '9') {
                binary_data |= nibble - '0';
            } else if (nibble >= 'A' && nibble <= 'F') {
                binary_data |= nibble - 'A';
            } else {
                puts("provided data must be hexadecimal");
                return 1;
            }
        }

        // Safe to re-use input data buffer
        data[i/2] = binary_data;
    }

    printf("Writing %u bytes of data at offset %u into data sector %u\n", (unsigned int)(data_len/2), offset_num, sector_num);
    size_t written = flash_data_write(sector_num, offset_num, (uint8_t*) data, data_len/2);
    printf(" ...wrote %u bytes of data\n", (unsigned int)written);
    return 0;

}

int run_flash_read(char * args) {

    const uint8_t READ_BUFFER_SIZE = 32;
    char read_buffer[READ_BUFFER_SIZE];

    char * sector = cli_get_token(&args);
    if (!sector) {
        puts("No sector num provided.");
        return 1;
    }

    char * offset = cli_get_token(&args);
    if (!offset) {
        puts("No read offset provided.");
        return 1;
    }

    char * len = cli_get_token(&args);
    if (!len) {
        puts("No read length provided.");
        return 1;
    }

    int sector_num = strtol(sector, NULL, 10);
    int offset_num = strtol(offset, NULL, 10);
    int len_num = strtol(len, NULL, 10);

    int total_read = 0;
    printf("Reading %u bytes of data at offset %u into data sector %u\n", len_num, offset_num, sector_num);
    printf(" DATA:");
    for (int i=0; i<len_num; i+=READ_BUFFER_SIZE) {
        size_t to_read = MIN(READ_BUFFER_SIZE, len_num-i);
        size_t did_read = flash_data_read(sector_num, offset_num, (uint8_t*)read_buffer, to_read);
        for (size_t j=0; j<did_read; j++) {
            putchar(read_buffer[j]);
        }
        total_read += (int) did_read;
    }
    printf("\r\n ...total bytes read: %u\n", total_read);
    return 0;
}

int run_flash_read_hex(char * args) {

    const uint8_t READ_BUFFER_SIZE = 32;
    char read_buffer[READ_BUFFER_SIZE];

    char * sector = cli_get_token(&args);
    if (!sector) {
        puts("No sector num provided.");
        return 1;
    }

    char * offset = cli_get_token(&args);
    if (!offset) {
        puts("No read offset provided.");
        return 1;
    }

    char * len = cli_get_token(&args);
    if (!len) {
        puts("No read length provided.");
        return 1;
    }

    int sector_num = strtol(sector, NULL, 10);
    int offset_num = strtol(offset, NULL, 10);
    int len_num = strtol(len, NULL, 10);

    int total_read = 0;
    printf("Reading %u bytes of data at offset %u into data sector %u\n", len_num, offset_num, sector_num);
    printf(" DATA:");
    for (int i=0; i<len_num; i+=READ_BUFFER_SIZE) {
        size_t to_read = MIN(READ_BUFFER_SIZE, len_num-i);
        size_t did_read = flash_data_read(sector_num, offset_num+i, (uint8_t*)read_buffer, to_read);
        for (size_t j=0; j<did_read; j++) {
            printf("%02x", read_buffer[j]);
        }
        total_read += did_read;
    }
    printf("\r\n ...total bytes read: %u\n", total_read);

    return 0;
}

int run_flash_erase(char * args) {
    char * sector = cli_get_token(&args);
    if (!sector) {
        return 1;
    }
    int sector_num = strtol(sector, NULL, 10);
    flash_erase(sector_num);
    printf("Erased data sector %u\n", sector_num);
    return 0;
}

int run_flash_erase_all(__attribute__((unused)) char * args) {
    flash_erase_all();
    puts("Erased all flash.");
    return 0;
}

static const CLI_COMMAND flash_subcommands[] = {
    {.name="read", .process=run_flash_read,
     .help="usage: flash read sector_num byte_offset byte_length"},
    {.name="read_hex", .process=run_flash_read_hex,
     .help="usage: flash read_hex sector_num byte_offset byte_length"},
    {.name="write", .process=run_flash_write,
     .help="usage: flash write sector_num byte_offset string_data"},
    {.name="write_hex", .process=run_flash_write_hex,
     .help="usage: flash write_hex sector_num byte_offset hex_data"},
    {.name="erase", .process=run_flash_erase,
     .help="usage: flash erase sector_num"},
    {.name="erase_all", .process=run_flash_erase_all,
     .help="usage: flash erase_all"},
    {},
};

const CLI_COMMAND flash_command = {
    .name="flash", .subcommands=(CLI_COMMAND *)flash_subcommands,
    .help="usage: flash subcommand [[args...]]\n"
          "valid subcommands: read read_hex write write_hex erase erase_all"
};
