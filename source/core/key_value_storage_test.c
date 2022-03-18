
/**
 * Test program for key-value storage.
 */

#include "key_value_storage.h"
#include <stdio.h>
#include <string.h>


int simple_test(void) {
    // Reset the flash, and then write some values and ensure they can be
    // read back fine.

    flash_kv_clear();

    // Try storing some values
    flash_kv_store_string("hello_string", "world_string");

    char test_binary_data[12] = "world_binary";
    flash_kv_store_binary("hello_binary", test_binary_data, 12);

    flash_kv_store_int("hello_int", (int)0xa5a5a5a5);

    // And get them back
    char output[20] = {0};

    bool successful = flash_kv_get_string("hello_string", output, 20);
    if (!successful) {
        printf("Failed to get hello world string\n");
        return 1;
    }

    if (0 != strcmp(output, "world_string")) {
        printf("Got incorrect hello world string: %s\n", output);
        return 1;
    }

    successful = flash_kv_get_binary("hello_binary", output, 20);
    if (!successful) {
        printf("Failed to get hello world binary\n");
        return 1;
    }

    // Since we included the null terminator in binary storage, this should work.
    if (0 != strcmp(output, "world_binary")) {
        printf("Got incorrect hello world binary: %s\n", output);
        return 1;
    }

    int output_int;
    successful = flash_kv_get_int("hello_int", &output_int);
    if (!successful) {
        printf("Failed to get hello world int\n");
        return 1;
    }

    if (output_int != (int)0xa5a5a5a5) {
        printf("Got unexpected hello world int: %08x\n", output_int);
        return 1;
    }

    return 0;
}

int overwrite_test(void) {

    flash_kv_clear();

    flash_kv_store_string("hello", "test1");

    // And get them back
    char output[20] = {0};

    bool successful = flash_kv_get_string("hello", output, 20);
    if (!successful) {
        printf("Failed to get test string\n");
        return 1;
    }
    if (0 != strcmp(output, "test1")) {
        printf("Failed first test string, got %s", output);
        return 1;
    }

    flash_kv_store_string("hello", "second test");
    successful = flash_kv_get_string("hello", output, 20);
    if (!successful) {
        printf("Failed to get second test string\n");
        return 1;
    }
    if (0 != strcmp(output, "second test")) {
        printf("Failed second test string, got %s", output);
        return 1;
    }

    flash_kv_store_string("hello", "test_the_third");
    successful = flash_kv_get_string("hello", output, 20);
    if (!successful) {
        printf("Failed to get third test string\n");
        return 1;
    }
    if (0 != strcmp(output, "test_the_third")) {
        printf("Failed third test string, got %s", output);
        return 1;
    }

    // Check after re-init: value is the last one
    flash_kv_init();
    successful = flash_kv_get_string("hello", output, 20);
    if (!successful) {
        printf("Failed to get third test string after reinit\n");
        return 1;
    }
    if (0 != strcmp(output, "test_the_third")) {
        printf("Failed third test string post reinit, got %s", output);
        return 1;
    }

    // Try deleting a value. We should no longer succesfully get it.
    flash_kv_delete("hello");
    successful = flash_kv_get_string("hello", output, 20);
    if (successful) {
        printf("Failed to delete key");
        return -1;
    }

    return 0;
}

void get_value(char* val, int i) {
    if (i%15) {
        sprintf(val, "val%d", i);
    }
    // occasionally write out a long value
    sprintf(val, "HELLO THIS IS A BIG LONG value full of characters and words"
                 "and such-things as to test long values getting copied");
}

int long_term_test(void) {

    flash_kv_clear();

    for (int i=0; i<4000; i++) {
        char key[15];
        char value[200];
        snprintf(key, 15, "key%d", i%10);
        get_value(value, i);

        if (i >= 10) {
            char old_value[200];
            get_value(old_value, i-10);
            char output[200] = {0};
            bool successful = flash_kv_get_string(key, output, 199);
            if (!successful) {
                printf("(%d), Lost an expected key: %s\n", i, key);
                return 1;
            }
            if (0 != strcmp(output, old_value)) {
                printf("(%d), Key %s had unexpected value: %s\n", i, key, output);
                return 1;
            }
        }

        flash_kv_store_string(key, value);

        if (i % 100 == 0) {
            flash_kv_init();
        }
    }


    return 0;
}


int main(void) {

    flash_kv_init();

    int result = 0;

    // A simple test where a few values get written and read back.
    printf("Running simple test:\n");
    result = simple_test();
    if (result) {
        printf("%u - simple test failed: %d\n", __LINE__, result);
        return 1;
    }

    // Test that saving a new value invalidates an old value.
    printf("Running value overwrite test:\n");
    result = overwrite_test();
    if (result) {
        printf("%u - value overwrite test failed: %d\n", __LINE__, result);
        return 1;
    }

    // Test that long-term usage, triggering things like cleanup and
    // moving between pages, works properly.
    printf("Running long-term test:\n");
    result = long_term_test();
    if (result) {
        printf("%u - long term test failed: %d\n", __LINE__, result);
        return 1;
    }


    printf("Tests passed.\n");

    return 0;
}