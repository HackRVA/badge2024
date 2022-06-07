//
// Created by Samuel Jones on 12/27/21.
//

#include "led_pwm.h"
#include "pinout_rp2040.h"
#include "hardware/pwm.h"
#include <stdio.h>

static uint8_t scale = 0xFF;

static const int _gpio_map[BADGE_LED_MAX] = {
    BADGE_GPIO_LED_RED,
    BADGE_GPIO_LED_GREEN,
    BADGE_GPIO_LED_BLUE,
    BADGE_GPIO_DISPLAY_BACKLIGHT,
};

static bool led_is_on[BADGE_LED_MAX];

void led_pwm_init_gpio() {
    for (int i=0; i<BADGE_LED_MAX; i++) {
        gpio_init(_gpio_map[i]);
        gpio_set_dir(_gpio_map[i], 1);
        gpio_put(_gpio_map[i], 1);
    }
}

void led_pwm_enable(BADGE_LED led, uint8_t duty) {
    if (led >= BADGE_LED_MAX) {
        return;
    }
    
    if (BADGE_LED_DISPLAY_BACKLIGHT == led) {
        // LED backlight has opposite pin polarity (active high)
        duty = ~duty;
    } else {
        // 3 color LED - apply scale.
        duty = (duty * scale) >> 8;
    }

    uint slice = pwm_gpio_to_slice_num(_gpio_map[led]);
    uint channel = pwm_gpio_to_channel(_gpio_map[led]);

    gpio_set_function(_gpio_map[led], GPIO_FUNC_PWM);
    pwm_set_enabled(slice, false);
    pwm_set_clkdiv_mode(slice, PWM_DIV_FREE_RUNNING);
    pwm_set_wrap(slice, 255);
    pwm_set_chan_level(slice, channel, duty);
    pwm_set_output_polarity(slice, true, true);
    pwm_set_enabled(slice, true);
    led_is_on[led] = true;
}

void led_pwm_disable(BADGE_LED led) {
    if (led >= BADGE_LED_MAX) {
        return;
    }

    uint slice = pwm_gpio_to_slice_num(_gpio_map[led]);
    uint channel = pwm_gpio_to_channel(_gpio_map[led]);

    pwm_set_chan_level(slice, channel, 256);
    gpio_init(_gpio_map[led]);
    gpio_set_dir(_gpio_map[led], 1);
    if (BADGE_LED_DISPLAY_BACKLIGHT != led) {
        gpio_put(_gpio_map[led], 1);
    }

    // Check to see if GPIOs are still on this slice. There are 4 potential
    // GPIOs for most slices.
    uint8_t slice_gpio_base = (slice * 2);
    bool still_using_slice = false;
    while (slice_gpio_base < 30) {
        if ((gpio_get_function(slice_gpio_base) == GPIO_FUNC_PWM) ||
            (gpio_get_function(slice_gpio_base+1) == GPIO_FUNC_PWM)) {
            still_using_slice = true;
            break;
        }

        slice_gpio_base += 16;
    }

    if (!still_using_slice) {
        pwm_set_enabled(slice, false);
    }

    led_is_on[led] = false;
}

bool led_pwm_is_on(BADGE_LED led) {

    if (led >= BADGE_LED_MAX) {
        return false;
    }

    return led_is_on[led];
}

void led_pwm_set_scale(uint8_t new_scale) {
    scale = new_scale;
}