/*!
 *  @file   analog_rp2040.c
 *  @author Peter Maxwell Warasila
 *  @date   April 2, 2024
 *
 *  @brief  RVASec Badge Analog Measurements Driver RP2040 Implemenation
 *
 *------------------------------------------------------------------------------
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <hardware/adc.h>

#include "analog.h"
#include "hardware/gpio.h"
#include "pinout_rp2040.h"

/*- Private Methods ----------------------------------------------------------*/
static uint16_t analog_get_adc_count(enum analog_channel channel)
{
    adc_select_input(channel);
    return adc_read();
}

/*- API ----------------------------------------------------------------------*/
void analog_init(void)
{
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

void analog_init_gpio(void)
{
    gpio_init(BADGE_GPIO_HALL_EFFECT_ENABLE);
    gpio_set_dir(BADGE_GPIO_HALL_EFFECT_ENABLE, true);
    gpio_set_input_enabled(BADGE_GPIO_HALL_EFFECT_ENABLE, true);
    gpio_disable_pulls(BADGE_GPIO_HALL_EFFECT_ENABLE); // saves power when asserted

    adc_gpio_init(BADGE_GPIO_ADC_CONDUCTIVITY);
    adc_gpio_init(BADGE_GPIO_ADC_THERMISTOR);
    adc_gpio_init(BADGE_GPIO_ADC_HALL_EFFECT);
    adc_gpio_init(BADGE_GPIO_ADC_BATTERY);
}

uint32_t analog_get_chan_mV(enum analog_channel channel)
{
    uint32_t count = analog_get_adc_count(channel);
    count *= 3300;
    count /= 4096;
    return count;
}

float analog_calc_resistance_ohms(uint32_t mV)
{
    return analog_calc_rdiv_bottom(3.3f, 2.2e3f, mV);
}

int8_t analog_calc_mcu_temp_C(uint32_t mV)
{
    float raw = mV;
    raw /= 1e3f;
    return 27 - ((raw - 0.706f) / 0.001721f);
}

int8_t analog_calc_thermistor_temp_C(uint32_t mV)
{
    const float R0_inv = 1.0f / 100e3f;
    const float B_inv = 1.0f / 4.2e3f;
    const float T0_inv = 1.0f / 298.15f;

    float R = analog_calc_rdiv_bottom(3.3f, 22e3f, mV);
    float K = 1.0f / (B_inv * log(R * R0_inv) + T0_inv);
    float C = K - 273.15f;
    return C;
}

int32_t analog_calc_hall_effect_mT(uint32_t _mV)
{
    int32_t mV = _mV;
    int32_t mT = 1000 - mV;
    mT /= 11; /* DRV5053OA is -11mV / mT */
    return mT;
}

enum analog_sensor_power analog_get_sensor_power(void)
{
    return gpio_get(BADGE_GPIO_HALL_EFFECT_ENABLE) ? ANALOG_SENSOR_POWER_DISABLED
                                                   : ANALOG_SENSOR_POWER_ENABLED;
}

void analog_set_sensor_power(enum analog_sensor_power power)
{
    /* Despite the naming, this signal is active low. */
    gpio_put(BADGE_GPIO_HALL_EFFECT_ENABLE,
            ANALOG_SENSOR_POWER_ENABLED == power ? false : true);
}
