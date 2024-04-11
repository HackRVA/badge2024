//
// Created by Sean DeArras on 4/09/24.
//

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


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
int16_t sample_buffer[256];
volatile int samples_read = 0;

void on_pdm_samples_ready()
{
    // callback from library when all the samples in the library
    // internal sample buffer are ready for reading
    samples_read = pdm_microphone_read(sample_buffer, 256);
}

void mic_init(void){
    pdm_microphone_init(&config);
    pdm_microphone_set_samples_ready_handler(on_pdm_samples_ready);
};

void mic_start(void){
    pdm_microphone_start();
};

void mic_stop(void){
    pdm_microphone_stop();
};

int16_t mic_get_qc_value(void){
    // returns max value of current sample buffer as a rough volume indicator

    int n = sizeof(sample_buffer) / sizeof(sample_buffer[0]);

    int16_t res = sample_buffer[0];

    for (int i = 1; i < n; i++) { 
        int16_t x = abs(sample_buffer[i]);
        res = res < x ? x : res;
    } 
    return res;
};