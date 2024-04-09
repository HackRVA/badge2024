
/*!
 *  @file   analog_sim.c
 *  @author Peter Maxwell Warasila
 *  @date   April 8, 2024
 *
 *  @brief  RVASec Badge Analog Measurements Driver Simulator Implemenation
 *
 *------------------------------------------------------------------------------
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "analog.h"

/*- Private Variables --------------------------------------------------------*/
static enum analog_sensor_power m_analog_sensor_power_state = ANALOG_SENSOR_POWER_DISABLED;

/*- API ----------------------------------------------------------------------*/
void analog_init(void)
{
    // nothing
}

void analog_init_gpio(void)
{
    // nothing
}

uint32_t analog_get_chan_mV(enum analog_channel channel)
{
    /* FIXME - implement channels. */
    switch (channel)
    {
        case ANALOG_CHAN_CONDUCTIVITY:
            return 3250; /* High-Z */
        case ANALOG_CHAN_THERMISTOR:
            return 2700; /* ~ 20 C */
        case ANALOG_CHAN_HALL_EFFECT:
            if (m_analog_sensor_power_state == ANALOG_SENSOR_POWER_ENABLED) {
                return 2000; /* +/- 0 T */
            } else {
                return 200; /* This seems to be what it decays to. -PMW */
            }
        case ANALOG_CHAN_BATT_V:
            return 2940;
        case ANALOG_CHAN_MCU_TEMP:
            return 706; /* 27 C */
        default:
            return 0;
    }
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
    return m_analog_sensor_power_state;
}

void analog_set_sensor_power(enum analog_sensor_power power)
{
    m_analog_sensor_power_state = power;
}

