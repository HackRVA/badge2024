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


static inline float analog_calc_rdiv_bottom(float refV, float Rtop, uint32_t mV)
{
    float V = mV;
    V *= 1e-3f;
    V /= refV;
    if (V == 0.0f) V = 0.0000001f; /* Guard zero div. */
    return Rtop * V / (1 - V);
}

float analog_calc_resistance_ohms(uint32_t mV);

static inline uint32_t analog_get_batt_mV(void) 
{
    return analog_get_chan_mV(ANALOG_CHAN_BATT_V);
}

int8_t analog_calc_mcu_temp_C(uint32_t mV);

int8_t analog_calc_thermistor_temp_C(uint32_t mV);

int32_t analog_calc_hall_effect_mT(uint32_t mV);

enum analog_sensor_power {
    ANALOG_SENSOR_POWER_DISABLED = 0,
    ANALOG_SENSOR_POWER_ENABLED,
};

/** Get sensor power rail enabled or disabled.
 *
 *  @return  value from enum analog_sensor_power.
 */
enum analog_sensor_power analog_get_sensor_power(void);

/** Set sensor power rail enabled or disabled.
 *
 *  @note   Must be enabled to measure hall effect sensor.
 *
 *  @param  power   Power enabled or disabled.
 */
void analog_set_sensor_power(enum analog_sensor_power power);

#if TARGET_SIMULATOR
/* For use by simulator, not badge apps */
struct analog_sim_values {
	int value[5]; /* indexed by enum analog_channel */
};

void analog_sensors_set_values(struct analog_sim_values values);

#endif
/*! @} */ // BADGE_ANALOG

#endif /* BADGE_C_ANALOG_H */

