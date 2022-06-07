//
// Created by Samuel Jones on 12/27/21.
//

#ifndef BADGE2022_C_LED_PWM_H
#define BADGE2022_C_LED_PWM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BADGE_LED_RGB_RED = 0,
    BADGE_LED_RGB_GREEN,
    BADGE_LED_RGB_BLUE,
    BADGE_LED_DISPLAY_BACKLIGHT,
    BADGE_LED_MAX,
} BADGE_LED;

void led_pwm_init_gpio();

void led_pwm_enable(BADGE_LED led, uint8_t duty);

void led_pwm_disable(BADGE_LED led);

/** @brief Tell us if the provided LED is on. */
bool led_pwm_is_on(BADGE_LED led);

// Sets LED scaling (range 0-255) on the 3-color LED.
void led_pwm_set_scale(uint8_t scale);

#endif //BADGE2022_C_LED_PWM_H
