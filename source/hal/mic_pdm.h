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
#include <audio.h>

#define MIC_CALLBACK_TABLE_SIZE (4) /*!< Number of microphone input callbacks simultaneously active. */

void mic_init(void);
void mic_start(void);
void mic_stop(void);

/*! Microphone input callback.
 *
 *  @param  samples Samples captured by the microphone.
 *  @param  len     Number of samples captured by the microphone.
 */
typedef void (*mic_callback_t)(const audio_sample_t *samples, size_t len);

/*! Add microphone input callback.
 *
 *  @param  cb  Callback to add.
 *
 *  @retval 0   Successfully added callback.
 *  @retval -1  Callback is NULL.
 *  @retval -2  Callback already in table.
 *  @retval -3  No more space in callback table.
 */
int mic_add_cb(mic_callback_t cb);

/*! Remove microphone input callback.
 *
 *  @param  cb  Callback to remove.
 *
 *  @retval 0   Callback removed.
 *  @retval -1  Callback specified is NULL.
 *  @retval -2  Callback not found in table.
 */
int mic_remove_cb(mic_callback_t cb);

#endif //BADGE_C_MIC_PDM_H

