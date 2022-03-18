//
// Created by Samuel Jones on 3/15/22.
//

/**
 * key_value_storage.c - a NOR-flash based key:value store for nonvolatile
 * data
 *
 * This module helps store arbitrary pieces of data to a collection of
 * NOR flash sectors. NOR flash has some oddities around how writes
 * happen and how data can be erased.
 *
 * Values are named by the key - which is a C string identifying the
 * data to be stored. The value to be stored or retrieved depends on
 * the function called.
 *
 */

#ifndef BADGE2022_C_KEY_VALUE_STORAGE_H
#define BADGE2022_C_KEY_VALUE_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

/// Maximum key length allowed.
#define MAX_KEY_LENGTH 64

/// Initialize key value storage. Call at startup to check data on flash.
bool flash_kv_init(void);

/// Clear key value storage.
bool flash_kv_clear(void);

/** Save binary data.
 *
 * This should be called only as needed to save data in nonvolatile memory -
 * if called every frame, it may cause flash to wear.
 *
 * There is a small chance this function will take a little while to
 * run in order to garbage collect, in a way.
 *
 * Returns true if storage was successful.
 */
bool flash_kv_store_binary(const char *key, const void* data, size_t len);

/// Save string data (see @ref flash_kv_store_binary);
bool flash_kv_store_string(const char *key, const char* value);

/// Save integer data (see @ref flash_kv_store_binary);
bool flash_kv_store_int(const char* key, int value);

/// Delete the data corresponding to the provided key, if it exists.
/// @return true if something was deleted.
bool flash_kv_delete(const char*key);

/** Get the binary data corresponding to the key.
 *
 * No more than `max_len` bytes will be retrieved.
 * @return the length of data that was retrieved. 0 if failure.
 */
size_t flash_kv_get_binary(const char* key, void* data, size_t max_len);

/** Get the string data corresponding to the key.
 *
 * No more than `max_len-1` data bytes will be retrieved. The string will
 * be null-terminated.
 * @return true if data was retrieved.
 */
bool flash_kv_get_string(const char* key, char* value, size_t max_len);

/** Get the integer data corresponding to the key.
 *
 * @return true if data was retrieved.
 */
bool flash_kv_get_int(const char* key, int* value);

#endif //BADGE2022_C_KEY_VALUE_STORAGE_H
