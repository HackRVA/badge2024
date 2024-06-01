//
// Created by Sean DeArras on 4/10/24.
//

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mic_pdm.h"

static mic_callback_t mic_cb_table[MIC_CALLBACK_TABLE_SIZE] = {0};

void mic_init(void){
}

void mic_start(void){
}

void mic_stop(void){
}

bool mic_running(){
	return false;
}


int mic_add_cb(mic_callback_t cb)
{
    if (NULL == cb) {
        return MIC_RC_EPARAM_NULL;
    }

    size_t available = SIZE_MAX;
    for (size_t i = 0; i < (sizeof(mic_cb_table) / sizeof(mic_cb_table[0])); i++) {
        if (mic_cb_table[i] == cb) {
            /* This callback is already used as an entry. */
            return MIC_RC_EALREADY_EXISTS;
        } else if ((NULL == mic_cb_table[i]) && (available == SIZE_MAX)) {
            available = i;
        }
    }

    if (available == SIZE_MAX) {
        /* There is no space in the table. */
        return MIC_RC_ENO_SPACE;
    }

    /* Assign the available entry. */
    mic_cb_table[available] = cb;
    return MIC_RC_OK;
}

int mic_remove_cb(mic_callback_t cb)
{
    if (NULL == cb) {
        return MIC_RC_EPARAM_NULL;
    }

    for (size_t i = 0; i < (sizeof(mic_cb_table) / sizeof(mic_cb_table[0])); i++) {
        if (mic_cb_table[i] == cb) {
            mic_cb_table[i] = NULL;
            return MIC_RC_OK;
        }
    }

    /* Did not find callback in table. */
    return MIC_RC_ENOT_THERE;
}

