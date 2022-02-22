//
// Created by Samuel Jones on 2/11/22.
//

#include "audio_output.h"

// Interface copied from old repository. This is likely to change!
void audio_set_note(uint16_t frequency, uint16_t duration) {
    // TODO: implement, or change interface!
    printf("Audio set note: freq %u, dur %u stub\n", frequency, duration);
}

void audio_next_note(void) {
    printf("Audio next note stub\n");
}

void audio_end_note(void) {
    printf("Audio end note stub\n");
}