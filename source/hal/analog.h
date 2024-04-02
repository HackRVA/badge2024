/*!
 *  @file   analog.h
 *  @author Peter Maxwell Warasila
 *  @date   April 2, 2024
 *
 *  @brief  RVASec Badge Analog Measurements Driver
 *
 *------------------------------------------------------------------------------
 *
 */

#ifndef BADGE_C_ANALOG_H
#define BADGE_C_ANALOG_H

/*! @defgroup   BADGE_ANALOG Analog Measurements Driver
 *  @{
 */

enum analog_channel {
    ANALOG_CHAN_CONDUCTIVITY    = 0,
    ANALOG_CHAN_THERMISTOR      = 1,
    ANALOG_CHAN_HALL_EFFECT     = 2,
    ANALOG_CHAN_BATT_V          = 3,
    ANALOG_CHAN_MCU_TEMP        = 4,
};

void analog_init(void);

void analog_init_gpio(void);

uint32_t analog_get_chan_mV(enum analog_channel chan);

static inline float analog_get_resistance_ohms(void)
{
    float raw = analog_get_chan_mV(ANALOG_CHAN_CONDUCTIVITY);
    raw /= 3.3e3f;
    return 2.2e3f * raw / (1 - raw);
}

static inline uint16_t analog_get_batt_mV(void) 
{
    return analog_get_chan_mV(ANALOG_CHAN_BATT_V);
}

static inline int8_t analog_get_mcu_temp_C(void)
{
    float raw = analog_get_chan_mV(ANALOG_CHAN_MCU_TEMP);
    raw /= 1e3f;
    return 27 - ((raw - 0.706f) / 0.001721f);
}

/*! @} */ // BADGE_ANALOG

#endif /* BADGE_C_ANALOG_H */

