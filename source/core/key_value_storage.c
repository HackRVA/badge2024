//
// Created by Samuel Jones on 3/15/22.
//

#include "key_value_storage.h"
#include "flash_storage.h"
#include <stdint.h>
#include <string.h>

enum {
    SECTOR_STATE_UNINIT = 0xFF,
    SECTOR_STATE_ACTIVE = 0x7F,
    SECTOR_STATE_INACTIVE = 0x3F,
};

enum {
    ENTRY_STATE_UNINIT = 0xFF,
    ENTRY_STATE_ALLOC = 0x7F,
    ENTRY_STATE_WRITTEN = 0x3F,
    ENTRY_STATE_DELETED = 0x1F,
};

const uint16_t KV_SECTOR_MAGIC = 0x6b76; // "kv"
const int KV_STORAGE_VERSION = 1;
const int KV_MAX_ENTRIES = 128;
const int KV_BASE_SECTOR = 0;
const int KV_SECTORS_PER_INSTANCE = 2;
const int KV_INSTANCES = 2;

// 8 bytes
typedef struct {
    uint16_t magic;
    uint8_t  state;
    uint8_t  version;
    uint16_t num_sectors;
    uint16_t num_entries;
} STORAGE_HEADER;

// 8 bytes
typedef struct {
    uint8_t state;
    uint8_t key_len;
    uint16_t value_len;
    uint16_t key_offset;
    uint16_t value_offset;
} ENTRY_HEADER;

static int current_base_sector = -1;
static int current_entries_used = 0;
static int current_free_data_offset = 0;
static STORAGE_HEADER current_storage_header;

static bool space_is_available(size_t size) {
    return (current_entries_used < current_storage_header.num_entries) &&
           (current_free_data_offset + size < (current_storage_header.num_sectors * FLASH_SECTOR_SIZE));
}

static void load_entry_header(ENTRY_HEADER *header, int sector, int entry_num) {
    flash_data_read(sector, sizeof(STORAGE_HEADER) + entry_num * sizeof(ENTRY_HEADER),
                    (uint8_t*)header, sizeof(ENTRY_HEADER));
}

static void save_entry_header(ENTRY_HEADER *header, int sector, int entry_num) {
    flash_data_write(sector, sizeof(STORAGE_HEADER) + entry_num * sizeof(ENTRY_HEADER),
                    (uint8_t*)header, sizeof(ENTRY_HEADER));
}

static bool load_storage_header(STORAGE_HEADER *header, int sector) {
    flash_data_read(sector, 0, (uint8_t*)header, sizeof(STORAGE_HEADER));
    return (header->magic == KV_SECTOR_MAGIC);
}

static bool find_key(const char* key, ENTRY_HEADER *entry, int sector, int used_entries, int* index) {

    for (int i=used_entries-1; i>=0; i--) {
        load_entry_header(entry, sector, i);
        if (entry->state != ENTRY_STATE_WRITTEN) {
            continue;
        }
        if (entry->key_len != strlen(key)) {
            continue;
        }
        if (entry->key_len >= MAX_KEY_LENGTH) {
            entry->key_len = MAX_KEY_LENGTH-1;
        }
        char entry_key[MAX_KEY_LENGTH];
        flash_data_read(sector, entry->key_offset, (uint8_t*)entry_key, entry->key_len);
        entry_key[entry->key_len] = '\0';

        if (0 == strcmp(key, entry_key)) {
            if (index) {
                *index = i;
            }
            return true;
        }
    }
    return false;
}

static void move_and_clean(void) {
    int new_sector_base = (current_base_sector == KV_BASE_SECTOR) ? KV_BASE_SECTOR + KV_SECTORS_PER_INSTANCE : KV_BASE_SECTOR;

    // Erase new area
    for (int i=0; i<KV_SECTORS_PER_INSTANCE; i++) {
        flash_erase(new_sector_base+i);
    }

    int new_entries_used = 0;
    int new_free_data_offset = sizeof(STORAGE_HEADER) + KV_MAX_ENTRIES * sizeof(ENTRY_HEADER);

    for (int i=current_entries_used; i>=0; i--) {
        ENTRY_HEADER entry_header;
        load_entry_header(&entry_header, current_base_sector, i);
        if (entry_header.state == ENTRY_STATE_WRITTEN) {
            char key[MAX_KEY_LENGTH];
            flash_data_read(current_base_sector, entry_header.key_offset, (uint8_t*)key, entry_header.key_len);
            key[entry_header.key_len] = '\0';

            // Migrate if and only if key doesn't exist already in the new area.
            ENTRY_HEADER dummy;
            if (find_key(key, &dummy, new_sector_base, new_entries_used, NULL)) {
                continue;
            }

            // Create new header
            ENTRY_HEADER new_entry = {
                .key_len = entry_header.key_len,
                .value_len = entry_header.value_len,
                .key_offset = new_free_data_offset,
                .value_offset = new_free_data_offset + entry_header.key_len,
                .state = ENTRY_STATE_WRITTEN,
            };
            save_entry_header(&new_entry, new_sector_base, new_entries_used);

            // Copy key
            flash_data_write(new_sector_base, new_free_data_offset, (uint8_t*)key, new_entry.key_len);

            // Copy value
            for (int j=0; j<new_entry.value_len; j+=32) {
                uint8_t value_data[32];
                size_t read_len = j + 32 >= new_entry.value_len ? new_entry.value_len - j : 32;
                flash_data_read(current_base_sector, entry_header.value_offset + j, value_data, read_len);
                flash_data_write(new_sector_base, new_entry.value_offset + j, value_data, read_len);
            }

            // Increment offsets
            new_entries_used++;
            new_free_data_offset += new_entry.value_len + new_entry.key_len;
        }
    }


    // Finalize - mark new area as ready and invalidate old one
    STORAGE_HEADER new_storage_header = {
            .magic = KV_SECTOR_MAGIC,
            .state = SECTOR_STATE_ACTIVE,
            .version = 1,
            .num_entries = KV_MAX_ENTRIES,
            .num_sectors = KV_SECTORS_PER_INSTANCE,
    };
    flash_data_write(new_sector_base, 0, (uint8_t*)&new_storage_header, sizeof(STORAGE_HEADER));

    current_storage_header.state = SECTOR_STATE_INACTIVE;
    flash_data_write(current_base_sector, 0, (uint8_t*)&current_storage_header, sizeof(STORAGE_HEADER));

    current_storage_header = new_storage_header;
    current_entries_used = new_entries_used;
    current_free_data_offset = new_free_data_offset;
    current_base_sector = new_sector_base;
}


void save_storage_header(const STORAGE_HEADER *header, int sector) {
    flash_data_write(sector, 0, (uint8_t*)header, sizeof(STORAGE_HEADER));
}

bool flash_kv_init(void) {

    STORAGE_HEADER header;
    bool found = false;

    for (int i=0; i<KV_INSTANCES*KV_SECTORS_PER_INSTANCE; i+=KV_SECTORS_PER_INSTANCE) {
        if (!load_storage_header(&header, i + KV_BASE_SECTOR)) {
            continue;
        }
        if (header.state == SECTOR_STATE_ACTIVE) {
            current_base_sector = i;
            current_storage_header = header;

            // Scan backward until we find used entry slots.
            current_entries_used = 0;
            current_free_data_offset = 0;
            ENTRY_HEADER entry_header;
            for (int j=header.num_entries-1; j>=0; j--) {

                load_entry_header(&entry_header, current_base_sector, j);

                if (entry_header.state != ENTRY_STATE_UNINIT && !current_entries_used) {
                    current_entries_used = j+1;
                }

                // Once we start finding entries, look for key:value data to determine the write
                // offset we should use
                if (current_entries_used) {
                    if (entry_header.value_offset != 0xFFFF) {
                        current_free_data_offset = entry_header.value_offset + entry_header.value_len;
                    }
                }

                // The location of the last entry should give us the correct offset to use.
                if (current_free_data_offset) {
                    break;
                }
            }
            found = true;
            break;
        }
    }

    // If we didn't find any entries, then we should start writing at the start of the data area
    if (!current_free_data_offset) {
        current_free_data_offset = (int)(header.num_entries * sizeof(ENTRY_HEADER) + sizeof(STORAGE_HEADER));
    }

    return found;
}

bool flash_kv_clear(void) {
    for (int i=0; i<NUM_DATA_SECTORS; i++) {
        flash_erase(i);
    }
    current_storage_header.magic = KV_SECTOR_MAGIC;
    current_storage_header.version = KV_STORAGE_VERSION;
    current_storage_header.state = SECTOR_STATE_ACTIVE;
    current_storage_header.num_entries = KV_MAX_ENTRIES;
    current_storage_header.num_sectors = KV_SECTORS_PER_INSTANCE;

    save_storage_header(&current_storage_header, 0);
    current_base_sector = 0;
    current_entries_used = 0;
    current_free_data_offset = sizeof(STORAGE_HEADER) + KV_MAX_ENTRIES * sizeof(ENTRY_HEADER);
    return true;
}

bool flash_kv_store_binary(const char *key, const void* value, size_t len) {
    size_t space_needed = strlen(key) + len;
    if (!space_is_available(space_needed)) {
        move_and_clean();
    }
    if (!space_is_available(space_needed)) {
        return false;
    }

    ENTRY_HEADER new;
    ENTRY_HEADER existing;
    int existing_index = -1;
    bool write_key = false;
    if (find_key(key, &existing, current_base_sector, current_entries_used, &existing_index)) {
        // Don't need to write key
        new.key_len = existing.key_len;
        new.key_offset = existing.key_offset;
    } else {
        new.key_len = strlen(key);
        new.key_offset = current_free_data_offset;
        current_free_data_offset += new.key_len;
        write_key = true;
    }

    new.value_len = len;
    new.value_offset = current_free_data_offset;
    current_free_data_offset += new.value_len;

    // Write header entry before data so we reserve the data in case of reset between ops
    new.state = ENTRY_STATE_ALLOC;
    save_entry_header(&new, current_base_sector, current_entries_used);

    if (write_key) {
        flash_data_write(current_base_sector, current_free_data_offset-new.value_len-new.key_len, (uint8_t*)key, new.key_len);
    }
    flash_data_write(current_base_sector, current_free_data_offset-new.value_len, value, new.value_len);

    // Update header entry to mark write as finished
    new.state = ENTRY_STATE_WRITTEN;
    save_entry_header(&new, current_base_sector, current_entries_used);

    // Mark old data as deleted. Not necessary to work, but will make clean operations faster.
    if (existing_index >= 0) {
        existing.state = ENTRY_STATE_DELETED;
        save_entry_header(&existing, current_base_sector, existing_index);
    }

    current_entries_used++;
    return true;
}


bool flash_kv_store_string(const char *key, const char* value) {
    size_t value_len = strlen(value);
    return flash_kv_store_binary(key, value, value_len);
}

bool flash_kv_store_int(const char* key, int value) {
    size_t value_len = sizeof(int);
    return flash_kv_store_binary(key, &value, value_len);
}

bool flash_kv_delete(const char* key) {
    ENTRY_HEADER entry_header;
    int index;
    if (find_key(key, &entry_header, current_base_sector, current_entries_used, &index)) {
        entry_header.state = ENTRY_STATE_DELETED;
        save_entry_header(&entry_header, current_base_sector, index);
        return true;
    }
    return false;
}

size_t flash_kv_get_binary(const char* key, void* value, size_t max_len) {
    if (current_base_sector < 0) {
        return 0;
    }
    ENTRY_HEADER entry;
    if (find_key(key, &entry, current_base_sector, current_entries_used, NULL)) {
        if (max_len >= entry.value_len) {
            max_len = entry.value_len;
        }
        return flash_data_read(current_base_sector, entry.value_offset, (uint8_t*)value, max_len);
    }
    return 0;
}

bool flash_kv_get_string(const char* key, char* value, size_t max_strlen) {

    size_t len = flash_kv_get_binary(key, value, max_strlen);
    value[len] = '\0';
    return len > 0;
}

bool flash_kv_get_int(const char* key, int* value) {
    size_t len = flash_kv_get_binary(key, value, sizeof(int));
    return len > 0;
}