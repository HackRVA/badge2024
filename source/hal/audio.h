/*!
 *  @file   audio.h
 *  @author Peter Maxwell Warasila
 *  @date   May 28, 2022
 *  
 *  @brief  RVASec 2022 Badge Audio Driver
 *
 *------------------------------------------------------------------------------
 * 
 */


#ifndef BADGE2022_C_AUDIO_H
#define BADGE2022_C_AUDIO_H

/*! @defgroup   BADGE2022_AUDIO Audio Driver
 *  @{
 */

#define AUDIO_DEPTH_BITS        (8U)                            //!< Audio driver bit depth
#define AUDIO_SAMPLE_MAX        ((1 << AUDIO_DEPTH_BITS) - 1)   //!< Audio driver maximum sample value

#define AUDIO_BEEP_FREQ_HZ_MIN  (120)
#define AUDIO_BEEP_FREQ_HZ_MAX  (10000)
#define AUDIO_BEEP_DUR_MS_MIN   (1)
#define AUDIO_BEEP_DUR_MS_MAX   (5000)

/*!
 *  @brief  Initialize and configure audio gpio
 *  
 *  @subsection Input
 *  The input is configured 
 */
void audio_init_gpio();

/*!
 *  @brief  Intialize audio driver
 */
void audio_init();

/*!
 *  @brief  Play an old fashioned beep on the speaker.
 *  
 *  @param  frequency   Frequency in Hertz
 *  @param  duration    Duration in milliseconds
 */
int audio_out_beep(uint16_t freq, uint16_t duration);

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

/*! @} */ // BADGE2022_AUDIO

#endif /* BADGE2022_C_AUDIO_H */
