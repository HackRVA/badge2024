//
// Created by Sean DeArras on 4/09/24.
//

#ifndef BADGE_C_MIC_PDM_H
#define BADGE_C_MIC_PDM_H

#include <audio.h>

#define MIC_CALLBACK_TABLE_SIZE (4) /*!< Number of microphone input callbacks simultaneously active. */

enum mic_rc {
    MIC_RC_OK = 0,              /*!< Completed successfully. */
    MIC_RC_EPARAM_NULL,         /*!< Parameter is NULL. */
    MIC_RC_EALREADY_EXISTS,     /*!< Callback is already in table. May be ignored if expected. */
    MIC_RC_ENO_SPACE,           /*!< No space left in table. */
    MIC_RC_ENOT_THERE,          /*!< Callback is not in table. May be ignored if expected. */
};

/*! Initialize the microphone. */
void mic_init(void);

/*! Manually start the microphone. */
void mic_start(void);

/*! Manually stop the microphone. */
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
 *  @note   Automatically starts microphone if not already running.
 *
 *  @retval MIC_RC_OK               Successfully added callback.
 *  @retval MIC_RC_EPARAM_NULL      Callback is NULL.
 *  @retval MIC_RC_EALREADY_EXISTS  Callback already in table.
 *  @retval MIC_RC_ENO_SPACE        No more space in callback table.
 */
int mic_add_cb(mic_callback_t cb);

/*! Remove microphone input callback.
 *
 *  @param  cb  Callback to remove.
 *
 *  @note   Automatically stops microphone if no callbacks remain.
 *
 *  @retval MIC_RC_OK           Callback removed successfully.
 *  @retval MIC_RC_EPARAM_NULL  Callback specified is NULL.
 *  @retval MIC_RC_ENOT_THERE   Callback not found in table.
 */
int mic_remove_cb(mic_callback_t cb);

#endif //BADGE_C_MIC_PDM_H

