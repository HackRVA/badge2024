# Audio

A simple audio beep can be accomplished with the following
```C
/*!
 *  @brief  Play an old fashioned beep on the speaker.
 *  
 *  @param  frequency   Frequency in Hertz
 *  @param  duration    Duration in milliseconds
 */
int audio_out_beep(uint16_t freq, uint16_t duration);
```
