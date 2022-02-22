//
// Created by Samuel Jones on 2/21/22.
//

#include "audio_output.h"

// Interface copied from old repository. This is likely to change!
void audio_set_note(uint16_t frequency, uint16_t duration) {
    // TODO: implement, or change interface!
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    printf("Audio set note: freq %u, dur %u stub\n", frequency, duration);
}

void audio_next_note(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}

void audio_end_note(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}