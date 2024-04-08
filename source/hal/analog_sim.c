
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

enum analog_sensor_power analog_get_sensor_power(void)
{
    return m_analog_sensor_power_state;
}

void analog_set_sensor_power(enum analog_sensor_power power)
{
    m_analog_sensor_power_state = power;
}

