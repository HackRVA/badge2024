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

enum color_sensor_error_code {
    COLOR_SENSOR_ERROR_NONE = 0,

    /* 0x000f reserved for error location. */
    COLOR_SENSOR_ERROR_LOCATION     = 0x000f,
    COLOR_SENSOR_ERROR_UP_PID       = 0x0001,
    COLOR_SENSOR_ERROR_UP_IFLG      = 0x0002,
    COLOR_SENSOR_ERROR_UP_POR       = 0x0003,
    COLOR_SENSOR_ERROR_UP_RTRY      = 0x0004,
    COLOR_SENSOR_ERROR_UP_RST       = 0x0005,
    COLOR_SENSOR_ERROR_UP_EN        = 0x0006,
    COLOR_SENSOR_ERROR_DOWN         = 0x0007,
    COLOR_SENSOR_ERROR_SAMP_EFLG    = 0x0008,
    COLOR_SENSOR_ERROR_SAMP_READ    = 0x0009,

    /* 0xfff0 reserved for error flags. */
    COLOR_SENSOR_ERROR_MEMRD_WRITE  = 0x0010,
    COLOR_SENSOR_ERROR_MEMRD_READ   = 0x0020,
    COLOR_SENSOR_ERROR_RGMOD_MEMRD  = 0x0040,
    COLOR_SENSOR_ERROR_RGMOD_WRITE  = 0x0080,
    COLOR_SENSOR_ERROR_I2C          = 0x0300,
    COLOR_SENSOR_ERROR_I2C_NOADACK  = 0x0100,
    COLOR_SENSOR_ERROR_I2C_TIMEOUT  = 0x0200,
    COLOR_SENSOR_ERROR_I2C_DATALEN  = 0x0400,
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

/** Get color sensor error code. */
enum color_sensor_error_code color_sensor_get_error_code(void);

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

#if TARGET_SIMULATOR
/* For use by the simulator, not badge apps */
void color_sensor_set_sample(struct color_sample sample);
#endif

/*! @} */ // BADGE_COLOR_SENSOR

#endif /* BADGE_C_COLOR_SENSOR_H */

