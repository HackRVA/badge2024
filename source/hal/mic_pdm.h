//
// Created by Sean DeArras on 4/09/24.
//

#ifndef BADGE_C_MIC_PDM_H
#define BADGE_C_MIC_PDM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void mic_init(void);
void mic_start(void);
void mic_stop(void);
int16_t mic_get_qc_value(void);

#endif //BADGE_C_MIC_PDM_H