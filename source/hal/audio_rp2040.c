/*!
 *  @file   audio_rp2040.c
 *  @author Peter Maxwell Warasila
 *  @date   May 28, 2022
 *  
 *  @brief  RVASec 2022 Badge Audio Implementation
 *
 *------------------------------------------------------------------------------
 * 
 *  @note   Only basic output beep implemented!
 * 
 */

#include <stdint.h>
#include <stdbool.h>

#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/irq.h>
#include <hardware/dma.h>
#include <hardware/adc.h>
#include <hardware/clocks.h>

#include "pinout_rp2040.h"

#include "audio.h"

/*! @addtogroup BADGE2022_AUDIO Audio Driver
 *  @{
 */


static unsigned slice;
static unsigned chan;

static volatile enum audio_out_mode_ {
    AUDIO_OUT_MODE_OFF = 0,
    AUDIO_OUT_MODE_BEEP,
} audio_out_mode;

static uint16_t test_wave[] = 
{
    127,133,140,146,152,158,163,169,173,177,181,184,187,189,190,
    190,190,189,188,186,183,179,175,171,166,161,155,149,143,136,
    130,123,117,110,104, 98, 92, 87, 82, 78, 74, 70, 67, 65, 64,
     63, 63, 63, 64, 66, 69, 72, 76, 80, 84, 90, 95,101,107,113,
    120
};
static int test_pos = 0;

/*- IRQ Handlers -------------------------------------------------------------*/

static void audio_out_pwm_irq_handler()
{
    pwm_clear_irq(slice);
    switch (audio_out_mode)
    {
        case AUDIO_OUT_MODE_OFF:
        case AUDIO_OUT_MODE_BEEP:
            /* this irq shouldn't be here anyway */
            pwm_set_irq_enabled(slice, false);
            return;
            
        default:
            return;
    }
#if 0
    int sample = test_wave[(test_pos++)>>0];
    // sample -= 127; // remove bias
    // sample *= 32;
    // sample /= 16;
    // sample += 128; // add bias
    pwm_set_chan_level(slice, chan, sample);
    if (test_pos >= (sizeof(test_wave)*1/sizeof(test_wave[0]))) test_pos = 0; 
#endif
}

static void audio_in_iqr_handler()
{
    uint16_t sample = adc_fifo_get();
    pwm_set_chan_level(slice, chan, sample);
}

/*- Initialization -----------------------------------------------------------*/

void audio_init_gpio()
{
    /* Initialize audio gpio */
    gpio_init(BADGE_GPIO_AUDIO_INPUT);

    /* Configure PWM slew rate, drive strength, and pwm slice/channel */
    gpio_init(BADGE_GPIO_AUDIO_PWM);
    gpio_disable_pulls(BADGE_GPIO_AUDIO_PWM);
    gpio_set_function(BADGE_GPIO_AUDIO_PWM, GPIO_FUNC_PWM);
    gpio_set_slew_rate(BADGE_GPIO_AUDIO_PWM, GPIO_SLEW_RATE_SLOW);
    gpio_set_drive_strength(BADGE_GPIO_AUDIO_PWM, GPIO_DRIVE_STRENGTH_2MA);
    slice = pwm_gpio_to_slice_num(BADGE_GPIO_AUDIO_PWM);
    chan = pwm_gpio_to_channel(BADGE_GPIO_AUDIO_PWM);
    
    /* Configure audio standby as open-drain, intially drain */
    gpio_init(BADGE_GPIO_AUDIO_STANDBY);
    gpio_disable_pulls(BADGE_GPIO_AUDIO_STANDBY);
    gpio_set_input_enabled(BADGE_GPIO_AUDIO_STANDBY, false);
    gpio_put(BADGE_GPIO_AUDIO_STANDBY, 0);
    gpio_set_dir(BADGE_GPIO_AUDIO_STANDBY, true);
}

static void audio_in_init()
{
    adc_init();
    adc_gpio_init(BADGE_GPIO_AUDIO_INPUT);
    adc_select_input(2);
    adc_fifo_setup(true, false, 1, false, true);
    adc_set_clkdiv(1000.0f); // 48 MHz / 1000 = 48kHz
    
    adc_irq_set_enabled(true);
    irq_set_exclusive_handler(ADC_IRQ_FIFO, &audio_in_iqr_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);

    // g_adc_dma_chan = dma_claim_unused_channel(true);
    // dma_channel_config config = dma_channel_get_default_config(g_adc_dma_chan);
    // channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    // channel_config_set_read_increment(&config, false);
    // channel_config_set_write_increment(&config, false);
    // channel_config_set_dreq(&config, DREQ_ADC);
    // dma_channel_configure(g_adc_dma_chan, &config, ((void *) pwm_hw->slice[slice].cc) + 2, &adc_hw->fifo, UINT32_MAX / 2, true);

    adc_run(true);

    return;
}

static void audio_out_init()
{
    pwm_set_enabled(slice, false);
    pwm_set_clkdiv_mode(slice, PWM_DIV_FREE_RUNNING);
    pwm_set_wrap(slice, AUDIO_SAMPLE_MAX - 1);
    pwm_set_chan_level(slice, chan, AUDIO_SAMPLE_MAX / 2);
    pwm_set_irq_enabled(slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, audio_out_pwm_irq_handler);
    //irq_set_enabled(PWM_IRQ_WRAP, true);
    //pwm_set_enabled(slice, true);

    /* Used for beep */
    alarm_pool_init_default();
}

void audio_init()
{
    audio_out_init();
    //audio_in_init();
}

/*- Standby Pin Control ------------------------------------------------------*/
void audio_stby_ctl(bool enable)
{
    /* Always take the opamp out of standby if requested */
    if (!enable)
    {
        gpio_set_dir(BADGE_GPIO_AUDIO_STANDBY, false);
    }
    /* Only put the opamp into standby if nothing is using it */
    else if (enable && (audio_out_mode == AUDIO_OUT_MODE_OFF))
    {
        gpio_set_dir(BADGE_GPIO_AUDIO_STANDBY, true);
    }
}

/*- Output -------------------------------------------------------------------*/
static int64_t audio_out_beep_alarm(__attribute__((unused)) alarm_id_t id,
                                    __attribute__((unused)) void* user_data)
{
    if (audio_out_mode != AUDIO_OUT_MODE_BEEP)
    {
        return 0;
    }

    audio_out_mode = AUDIO_OUT_MODE_OFF;
    pwm_set_enabled(slice, false);
    audio_stby_ctl(true);

    return 0;
}

int audio_out_beep(uint16_t freqHz, uint16_t durMs)
{
    if ((freqHz < AUDIO_BEEP_FREQ_HZ_MIN)
        || (freqHz > AUDIO_BEEP_FREQ_HZ_MAX)
        || (durMs < AUDIO_BEEP_DUR_MS_MIN)
        || (durMs > AUDIO_BEEP_DUR_MS_MAX))
    {
        return -1;
    }

    static alarm_id_t prev_alarm;
    if (AUDIO_OUT_MODE_BEEP == audio_out_mode)
    {
        alarm_pool_cancel_alarm(alarm_pool_get_default(), prev_alarm);
    }
    audio_out_mode = AUDIO_OUT_MODE_BEEP;
    audio_stby_ctl(false);
    pwm_set_enabled(slice, false);
    pwm_set_wrap(slice, 4096);
    pwm_set_clkdiv(slice, (float) clock_get_hz(clk_sys) / (float) freqHz / 4096.0f);
    pwm_set_chan_level(slice, chan, 4096 / 2);
    pwm_set_enabled(slice, true);
    prev_alarm = alarm_pool_add_alarm_in_ms(alarm_pool_get_default(),
                                            durMs,
                                            audio_out_beep_alarm,
                                            NULL,
                                            true);
    return 0;
}

bool audio_is_playing(void) {
    return audio_out_mode == AUDIO_OUT_MODE_BEEP;
}

/*! @} */ // BADGE2022_AUDIO
