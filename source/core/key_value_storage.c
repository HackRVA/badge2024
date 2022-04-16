//
// Created by Samuel Jones on 3/15/22.
//

/**
 * @file key_value_storage.c
 * @brief Key-value storage module implementation.
 *
 * This key-value storage module works under the following model of NOR flash:
 *
 * 1) Data can be read and written randomly.
 * 2) Erases can only happen on a sector and not a smaller unit.
 * 3) Erased flash reads back 0xFF, and writes only bits that are 1 to 0,
 *    not the reverse (Writes can't turn 0s into 1s).
 * 4) Following from (1) and (3), It's fine to write a location that has
 *    already been written, but the resulting value will be the logical
 *    AND of the old data and the new data.
 *
 * The structure implemented here has the following properties:
 *
 * 1) There are at least two "instances" or NOR flash locations where
 *    data are be stored. One is considered "active".
 * 2) When data is written, it is written into bare areas of the
 *    active area.
 * 3) Eventually, the instance area will become too full to write
 *    data. When that happens, the module will erase an inactive area,
 *    and copy all data that is not old or deleted from the active
 *    area to the new area. This will clean up old values that are
 *    no longer used, and should free enough space. This new area
 *    becomes the active area.
 *
 * An instance has the following layout:
 *  [storage header (8 bytes)]
 *  [entry table (8 bytes * maximum entries)]
 *  [data (rest of area)]
 *
 * The storage header holds version information, a magic value, and
 * metadata.
 *
 * The entry table holds information on where a key/value are stored
 * and if this record is unwritten, valid, or outdated/deleted.
 *
 * The data table just stores keys and values.
 *
 * Care is taken to try to ensure that if an inopportune reset happens
 * between internal writes/erases, data isn't be corrupted or lost.
 */

#include "key_value_storage.h"
#include "flash_storage.h"
#include <stdint.h>
#include <string.h>

/// This is a magic value that should be at the start of an initialized instance.
const uint16_t KV_SECTOR_MAGIC = 0x6b76; // "kv"

/// A version number so we can tell the data structure on flash, if it
/// becomes necessary.
const int KV_STORAGE_VERSION = 1;

/// The first flash page sector that stores key-value data. (The nor flash
/// driver puts sector 0 in a more proper location - not actually address 0.)
const int KV_BASE_SECTOR = 0;

/// The number of flash sectors in a given area.
const int KV_SECTORS_PER_INSTANCE = 2;

/// The number of instances the module cycles through. Should be 2. With some
/// code changes it could be greater than 2.
const int KV_INSTANCES = 2;

/// The number of values (including old/overwritten) values that can be stored
/// in an instance before needing to clean up. This is in addition to general
/// storage requirements.
const int KV_MAX_ENTRIES = 128;

/// Sector states. Note that a sector begins in UNINIT state after erase, and
/// can only progress down the enum until the next time it is erased.
enum SectorState {
    SECTOR_STATE_UNINIT = 0xFF,    /// Sector hasn't yet been set up.
    SECTOR_STATE_ACTIVE = 0x7F,    /// Sector is set up and is currently being used.
    SECTOR_STATE_INACTIVE = 0x3F,  /// Sector was used in the past, but has old data.
};

/// 8 bytes of flash data that gets stored at the beginning of a storage instance.
struct StorageHeader {
    /// set to KV_SECTOR_MAGIC as part of setup.
    uint16_t magic;
    /// set to a SECTOR_STATE value.
    uint8_t  state;
    /// Storage version number. New instances will be KV_STORAGE_VERSION.
    uint8_t  version;
    /// Number of physical sectors in this storage instance.
    uint16_t num_sectors;
    /// Maximum number of entries allowed. This dictates the size of the entry table.
    uint16_t num_entries;
};


/// Entry states. Note that a sector begins in UNINIT state after erase, and
/// can only progress down the enum until the next time it is erased.
enum EntryState {
    ENTRY_STATE_UNINIT = 0xFF,
    // Used to mark an entry as "currently being written". So in case of
    // reset or power failure, we detect this don't use the value, and don't
    // re-use data that may have been pointed to here.
    ENTRY_STATE_ALLOC = 0x7F,
    ENTRY_STATE_WRITTEN = 0x3F,
    ENTRY_STATE_DELETED = 0x1F,
};

/// 8 bytes of data corresponding to where entry data can be found.
struct EntryHeader {
    /// set to an ENTRY_STATE value.
    uint8_t state;
    /// Length of key string (not including null-termination).
    uint8_t key_len;
    /// Length of value data.
    uint16_t value_len;
    /// Physical offset of the key from the start of the instance.
    uint16_t key_offset;
    /// Physical offset of teh value from the start of the instance.
    uint16_t value_offset;
};

/// The sector number that is the start of the current instance.
static int current_base_sector = -1;
/// The number of entry records that are used.
static int current_entries_used = 0;
/// The start of the empty portion of the data area, where new data can be written.
static int current_free_data_offset = 0;
/// Header/status information for the currently active instance.
static struct StorageHeader current_storage_header;

/// returns true if we have space available for the requested size of data.
static bool space_is_available(size_t size) {
    return (current_entries_used < current_storage_header.num_entries) &&
           (current_free_data_offset + size < (current_storage_header.num_sectors * FLASH_SECTOR_SIZE));
}

/// Load entry information from flash given the sector and location in the entry table.
static void load_entry_header(struct EntryHeader *header, int sector, int entry_num) {
    flash_data_read(sector, sizeof(struct StorageHeader) + entry_num * sizeof(struct EntryHeader),
                    (uint8_t*)header, sizeof(struct EntryHeader));
}

/// Save entry information to flash at the provided sector and entry number.
static void save_entry_header(struct EntryHeader *header, int sector, int entry_num) {
    flash_data_write(sector, sizeof(struct StorageHeader) + entry_num * sizeof(struct EntryHeader),
                    (uint8_t*)header, sizeof(struct EntryHeader));
}

/// Load the provided sector's header information.
static bool load_storage_header(struct StorageHeader *header, int sector) {
    flash_data_read(sector, 0, (uint8_t*)header, sizeof(struct StorageHeader));
    return (header->magic == KV_SECTOR_MAGIC);
}

/// Save header information to the provided sector.
static void save_storage_header(const struct StorageHeader *header, int sector) {
    flash_data_write(sector, 0, (uint8_t*)header, sizeof(struct StorageHeader));
}

/// Find a key in the storage area. Returns true if it was found.
static bool find_key(const char* key, struct EntryHeader *entry, int sector, int used_entries, int* index) {

    // Scan through entries to see if we have a match for this key.
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

/// This function moves valid data from a full instance to a new instance. Old or invalid
/// data is discarded.
static void move_and_clean(void) {
    int new_sector_base = (current_base_sector == KV_BASE_SECTOR) ?
            KV_BASE_SECTOR + KV_SECTORS_PER_INSTANCE : KV_BASE_SECTOR;

    // Erase new area
    for (int i=0; i<KV_SECTORS_PER_INSTANCE; i++) {
        flash_erase(new_sector_base+i);
    }

    int new_entries_used = 0;
    int new_free_data_offset = sizeof(struct StorageHeader) + KV_MAX_ENTRIES * sizeof(struct EntryHeader);

    for (int i=current_entries_used; i>=0; i--) {
        struct EntryHeader entry_header;
        load_entry_header(&entry_header, current_base_sector, i);
        if (entry_header.state == ENTRY_STATE_WRITTEN) {
            char key[MAX_KEY_LENGTH];
            flash_data_read(current_base_sector, entry_header.key_offset, (uint8_t*)key, entry_header.key_len);
            key[entry_header.key_len] = '\0';

            // Migrate if and only if key doesn't exist already in the new area.
            struct EntryHeader dummy;
            if (find_key(key, &dummy, new_sector_base, new_entries_used, NULL)) {
                continue;
            }

            // Create new header
            struct EntryHeader new_entry = {
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
    struct StorageHeader new_storage_header = {
            .magic = KV_SECTOR_MAGIC,
            .state = SECTOR_STATE_ACTIVE,
            .version = 1,
            .num_entries = KV_MAX_ENTRIES,
            .num_sectors = KV_SECTORS_PER_INSTANCE,
    };
    flash_data_write(new_sector_base, 0, (uint8_t*)&new_storage_header, sizeof(struct StorageHeader));

    current_storage_header.state = SECTOR_STATE_INACTIVE;
    flash_data_write(current_base_sector, 0, (uint8_t*)&current_storage_header, sizeof(struct StorageHeader));

    // Update static variables to point to new data.
    current_storage_header = new_storage_header;
    current_entries_used = new_entries_used;
    current_free_data_offset = new_free_data_offset;
    current_base_sector = new_sector_base;
}

// Load and check flash data, initializing RAM data from flash state. If there
// is no data on flash, set it up as new flash storage.
bool flash_kv_init(void) {

    struct StorageHeader header;
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
            struct EntryHeader entry_header;
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
        current_free_data_offset = (int)(header.num_entries * sizeof(struct EntryHeader) + sizeof(struct StorageHeader));
    }

    return found;
}

bool flash_kv_clear(void) {
    for (int i=0; i<KV_SECTORS_PER_INSTANCE*KV_INSTANCES; i++) {
        flash_erase(KV_BASE_SECTOR + i);
    }
    current_storage_header.magic = KV_SECTOR_MAGIC;
    current_storage_header.version = KV_STORAGE_VERSION;
    current_storage_header.state = SECTOR_STATE_ACTIVE;
    current_storage_header.num_entries = KV_MAX_ENTRIES;
    current_storage_header.num_sectors = KV_SECTORS_PER_INSTANCE;

    save_storage_header(&current_storage_header, KV_BASE_SECTOR);
    current_base_sector = 0;
    current_entries_used = 0;
    current_free_data_offset = sizeof(struct StorageHeader) + KV_MAX_ENTRIES * sizeof(struct EntryHeader);
    return true;
}

bool flash_kv_store_binary(const char *key, const void* value, size_t len) {
    size_t space_needed = strlen(key) + len;
    if (!space_is_available(space_needed)) {
        move_and_clean();
    }
    if (!space_is_available(space_needed)) {
        // Failed to make enough space :(
        return false;
    }

    struct EntryHeader new;
    struct EntryHeader existing; // need to see if there was old data so we can mark it as deleted.
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

    // Mark old data as deleted. This isn't strictly necessary, but will make clean operations faster.
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
    struct EntryHeader entry_header;
    int index;
    if (find_key(key, &entry_header, current_base_sector, current_entries_used, &index)) {
        entry_header.state = ENTRY_STATE_DELETED;
        save_entry_header(&entry_header, current_base_sector, index);
        return true;
    }
    return false;
}

size_t flash_kv_get_binary(const char* key, void* value, size_t max_len) {
    if (current_base_sector < KV_BASE_SECTOR) {
        return 0;
    }
    struct EntryHeader entry;
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