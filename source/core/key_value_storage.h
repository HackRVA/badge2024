//
// Created by Samuel Jones on 3/15/22.
//

#ifndef BADGE2022_C_KEY_VALUE_STORAGE_H
#define BADGE2022_C_KEY_VALUE_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_KEY_LENGTH 64

bool flash_kv_init(void);
bool flash_kv_clear(void);

bool flash_kv_store_binary(const char *key, const void* data, size_t len);
bool flash_kv_store_string(const char *key, const char* value);
bool flash_kv_store_int(const char* key, int value);
bool flash_kv_delete(const char*key);

size_t flash_kv_get_binary(const char* key, void* data, size_t max_len);
bool flash_kv_get_string(const char* key, char* value, size_t max_len);
bool flash_kv_get_int(const char* key, int* value);

#endif //BADGE2022_C_KEY_VALUE_STORAGE_H
