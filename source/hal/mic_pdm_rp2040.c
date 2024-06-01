//
// Created by Sean DeArras on 4/09/24.
//

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio.h"
#include "mic_pdm.h"

#include "pico/stdlib.h"
#include "pico/pdm_microphone.h"

// configuration
const struct pdm_microphone_config config = {
    // GPIO pin for the PDM DAT signal
    .gpio_data = 3,

    // GPIO pin for the PDM CLK signal
    .gpio_clk = 4,

    // PIO instance to use
    .pio = pio1,

    // PIO State Machine instance to use
    .pio_sm = 0,

    // sample rate in Hz
    .sample_rate = 8000,

    // number of samples to buffer
    .sample_buffer_size = 256,
};

// variables
static int16_t sample_buffer[256];
static volatile int samples_read = 0;
static bool mic_running = false;
static int mic_cb_count = 0;
static mic_callback_t mic_cb_table[MIC_CALLBACK_TABLE_SIZE] = {0};

static void on_pdm_samples_ready()
{
    samples_read = pdm_microphone_read(sample_buffer, 256);
    for (size_t i = 0; i < (sizeof(mic_cb_table) / sizeof(mic_cb_table[0])); i++) {
        if (NULL != mic_cb_table[i]) {
            mic_cb_table[i](sample_buffer, samples_read);
        }
    }
}

void mic_init(void){
    pdm_microphone_init(&config);
    pdm_microphone_set_filter_gain(2);
    pdm_microphone_set_samples_ready_handler(on_pdm_samples_ready);
};

void mic_start(void){
    if (mic_running) {
        printf("\r\nmic already running");
        return;
    }
    pdm_microphone_start();
    mic_running = true;
    printf("\r\nstarted mic");
};

void mic_stop(void){
    if (!mic_running) {
        printf("\r\nmic already stopped");
        return;
    }
    pdm_microphone_stop();
    mic_running = false;
    printf("\r\nstopped mic");
};

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
    mic_cb_count++;

    /* Automagically start microphone if not running. */
    if (!mic_running) {
        mic_start();
    }

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
            mic_cb_count--;

            /* Automagically stop mic if there are no cbs remaining. */
            if ((mic_cb_count == 0) && (mic_running)) {
                mic_stop();
            }

            return MIC_RC_OK;
        }
    }

    /* Did not find callback in table. */
    return MIC_RC_ENOT_THERE;
}

