//
// Created by Samuel Jones on 1/29/22.
//

#include "button.h"
#include "pinout_rp2040.h"
#include <stdint.h>

static const int8_t _button_gpios[BADGE_BUTTON_MAX] = {
    BADGE_GPIO_DPAD_LEFT,
    BADGE_GPIO_DPAD_DOWN,
    BADGE_GPIO_DPAD_UP,
    BADGE_GPIO_DPAD_RIGHT,
    BADGE_GPIO_ENCODER_SW,
};

void button_init_gpio(void) {
    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        gpio_init(_button_gpios[i]);
        gpio_set_input_enabled(_button_gpios[i], 1);
        gpio_pull_up(_button_gpios[i]);
    }
}

int button_poll(BADGE_BUTTON button) {
    return !gpio_get(_button_gpios[button]);
}

int button_mask(void) {
    int mask = 0;
    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        mask |= (button_poll(i)<<i);
    }
    return mask;
}
