#ifndef assets_h
#define assets_h

#define NUM_AUDIO_CHANNELS 2
#define BYTES_PER_LINE (NUM_AUDIO_CHANNELS * sizeof(float) + 1)

/** Muze buzzer */

/** Stops playback of the current audio asset */
void haltPlayback(void);

/** Makes assetId active and calls play routine
 *
 * @param [in] assetId ID of asset to draw
 */
void playAsset(unsigned char assetId);

/** Makes assetId active and calls draw routine
 *
 * @param [in] assetId ID of asset to draw
 */
void drawAsset(unsigned char assetId);
/** Callback for drawing 1-bit-per-pixel (mono) images.
 *
 * @param [in] assetId ID of image asset
 * @param frame unused
 */
void drawLCD1(unsigned char assetId, int frame);
/** Callback for drawing 2-bits-per-pixel (4-color) images.
 *
 * @param [in] assetId ID of image asset
 * @param frame unused
 */
void drawLCD2(unsigned char assetId, int frame);
/** Callback for drawing 4-bits-per-pixel (16-color) images.
 *
 * @param [in] assetId ID of image asset
 * @param frame unused
 */
void drawLCD4(unsigned char assetId, int frame);
/** Callback for drawing 8-bits-per-pixel (256-color) images.
 *
 * @param [in] assetId ID of image asset
 * @param frame unused
 */
void drawLCD8(unsigned char assetId, int frame);
/** Toggles the electric current to speaker if set number of ticks have passed.
 * If the current note is done, calls callback for the current audio asset.
 */
void doAudio();

#endif
