/*!
 *  @file   color_sensor.h
 *  @author Peter Maxwell Warasila
 *  @date   April 2, 2024
 *
 *  @brief  RVASec Badge Color Sensor Driver Interface
 *
 *------------------------------------------------------------------------------
 *
 */

#ifndef BADGE_C_COLOR_SENSOR_H
#define BADGE_C_COLOR_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

/*! @defgroup   BADGE_COLOR_SENSOR Color Sensor Driver
 *  @{
 */

enum color_sensor_state {
    COLOR_SENSOR_STATE_NO_INIT = 0,
    COLOR_SENSOR_STATE_READY,
    COLOR_SENSOR_STATE_POWER_DOWN,
    COLOR_SENSOR_STATE_ERROR = -1,
};

enum color_sensor_power_cmd {
    COLOR_SENSOR_POWER_CMD_UP,
    COLOR_SENSOR_POWER_CMD_DOWN,
};

enum color_sample_index {
    COLOR_SAMPLE_INDEX_RED      = 0,
    COLOR_SAMPLE_INDEX_GREEN    = 1,
    COLOR_SAMPLE_INDEX_BLUE     = 2,
    COLOR_SAMPLE_INDEX_WHITE    = 3,
    COLOR_SAMPLE_INDEX_IR       = 4,

    COLOR_SAMPLE_INDEX_COUNT    = 5
};

struct color_sample {
    uint8_t error_flags;
    uint16_t rgbwi[5];
};

/** Initialize the color sensor driver. */
void color_sensor_init(void);

/** Get current state of the color sensor. */
enum color_sensor_state color_sensor_get_state(void);

/** Power down or power up the color sensor.
 *  
 *  @param  cmd Power command for the driver.
 *
 *  @retval COLOR_SENSOR_POWER_STATE_ON     Powered up and ready.
 *  @retval COLOR_SENSOR_POWER_STATE_OFF    Powered down.
 */
enum color_sensor_state color_sensor_power_ctl(enum color_sensor_power_cmd cmd);

/** Get color sample from color sensor */
int color_sensor_get_sample(struct color_sample *sample);

/*! @} */ // BADGE_COLOR_SENSOR

#endif /* BADGE_C_COLOR_SENSOR_H */

