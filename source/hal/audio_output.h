//
// Created by Samuel Jones on 2/11/22.
//

#include <stdint.h>
#include <stdio.h>

#ifndef BADGE2022_C_AUDIO_OUTPUT_H
#define BADGE2022_C_AUDIO_OUTPUT_H

// Interface copied from old repository. This is likely to change!
void audio_set_note(uint16_t frequency, uint16_t duration);

void audio_next_note(void);

void audio_end_note(void);

#endif //BADGE2022_C_AUDIO_OUTPUT_H
