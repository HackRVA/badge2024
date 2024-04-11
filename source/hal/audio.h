/*!
 *  @file   audio.h
 *  @author Peter Maxwell Warasila
 *  @date   May 28, 2022
 *
 *  @brief  RVASec Badge Audio Driver
 *
 *------------------------------------------------------------------------------
 *
 */


#ifndef BADGE_C_AUDIO_H
#define BADGE_C_AUDIO_H

#include <stdint.h>
#include <stddef.h>

/*! @defgroup   BADGE_AUDIO Audio Driver
 *  @{
 */

typedef int16_t audio_sample_t;
#define AUDIO_SAMPLE_MAX        (INT16_MAX)     //!< Audio driver maximum sample value
#define AUDIO_SAMPLE_MIN        (INT16_MIN)     //!< Audio driver maximum sample value

#define AUDIO_BEEP_FREQ_HZ_MIN  (120)
#define AUDIO_BEEP_FREQ_HZ_MAX  (10000)
#define AUDIO_BEEP_DUR_MS_MIN   (1)
#define AUDIO_BEEP_DUR_MS_MAX   (30000)

/*!
 *  @brief  Initialize and configure audio gpio
 *
 *  @subsection Input
 *  The input is configured
 */
void audio_init_gpio(void);

/*!
 *  @brief  Intialize audio driver
 */
void audio_init(void);

/*!
 *  @brief  Play an old fashioned beep on the speaker.
 *
 *  @param  frequency   Frequency in Hertz
 *  @param  duration    Duration in milliseconds
 */
int audio_out_beep(uint16_t freq, uint16_t duration);

/*!
 *  @brief  Play an old fashioned beep on the speaker.
 *
 *  @param  frequency      Frequency in Hertz
 *  @param  duration       Duration in milliseconds
 *  @param  beep_finished  function to call when beep is finished playing.
 *
 */
int audio_out_beep_with_cb(uint16_t freq, uint16_t duration, void (*beep_finished)(void));

/*!
 *  @brief  Request the opamp standby pin take a certain state.
 *
 *  @param  enable  Request the standby mode be enabled
 */
void audio_stby_ctl(bool enable);

/*!
 * @brief Tell us Signal if the audio is on or not.
 */
bool audio_is_playing(void);

/** Get the RMS level.
 *
 *  @param  samples Pointer to buffer of samples.
 *  @param  len     Length of buffer of samples.
 *
 *  @return RMS level of samples.
 */
audio_sample_t audio_rms(audio_sample_t *samples, size_t len);

/** Get the peak level.
 *
 *  @param  samples Pointer to buffer of samples.
 *  @param  len     Length of buffer of samples.
 *
 *  @return Peak level of samples.
 */
audio_sample_t audio_peak(audio_sample_t *samples, size_t len);

/** Get ratio in dB.
 *
 *  @param  ref Reference level.
 *  @param  raw Raw level to compare.
 *
 *  @return Ratio in dB (20 log).
 */
int8_t audio_dB(audio_sample_t ref, audio_sample_t raw);

/** Get dBFS.
 *
 *  @param  raw Raw level to compare.
 *
 *  @return dBFS (20 log).
 */
static inline int8_t audio_dBFS(audio_sample_t raw)
{
    return audio_dB(AUDIO_SAMPLE_MAX, raw);
}

/*! @} */ // BADGE_AUDIO

#endif /* BADGE_C_AUDIO_H */
