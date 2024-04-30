/*!
 *  @file   color_sensor_sim.c
 *  @author Peter Maxwell Warasila
 *  @date   April 8, 2024
 *
 *  @brief  RVASec Badge Color Sensor Driver Implementation - Simulator
 *
 *------------------------------------------------------------------------------
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include "color_sensor.h"

/*- Private Variables --------------------------------------------------------*/
static enum color_sensor_state m_color_sensor_state = COLOR_SENSOR_STATE_NO_INIT;
static struct color_sample current_color_sample = {
	.error_flags = 0,
	.rgbwi = { 100, 100, 100, 100, 100 },
};

/*- Public API ---------------------------------------------------------------*/
void color_sensor_init(void)
{
    m_color_sensor_state = COLOR_SENSOR_STATE_READY;
}

enum color_sensor_state color_sensor_get_state(void)
{
    return m_color_sensor_state;
}

enum color_sensor_error_code color_sensor_get_error_code(void)
{
    return COLOR_SENSOR_ERROR_NONE;
}

enum color_sensor_state color_sensor_power_ctl(enum color_sensor_power_cmd cmd)
{
    enum color_sensor_state state = color_sensor_get_state();
    
    if ((COLOR_SENSOR_POWER_CMD_UP == cmd)
        && (COLOR_SENSOR_STATE_READY != state)) {
        m_color_sensor_state = COLOR_SENSOR_STATE_READY;
    } else if ((COLOR_SENSOR_POWER_CMD_DOWN == cmd)
               && (COLOR_SENSOR_STATE_POWER_DOWN != state)) {
        m_color_sensor_state = COLOR_SENSOR_STATE_POWER_DOWN;
    }
    
    return color_sensor_get_state();
}


int color_sensor_get_sample(struct color_sample *sample)
{
    *sample = current_color_sample;
    return 0;
}

void color_sensor_set_sample(struct color_sample sample)
{
	current_color_sample = sample;
}

