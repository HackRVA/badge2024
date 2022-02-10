//
// Created by Samuel Jones on 1/29/22.
//

#ifndef BADGE2022_C_BUTTON_H
#define BADGE2022_C_BUTTON_H

#include <stdbool.h>

typedef enum {
    BADGE_BUTTON_LEFT = 0,
    BADGE_BUTTON_DOWN,
    BADGE_BUTTON_UP,
    BADGE_BUTTON_RIGHT,
    BADGE_BUTTON_SW,
    BADGE_BUTTON_ENCODER_A,
    BADGE_BUTTON_ENCODER_B,
    BADGE_BUTTON_MAX
} BADGE_BUTTON;


void button_init_gpio(void);

// Poll button state. Returns 1 if pressed, 0 if not pressed
int button_poll(BADGE_BUTTON button);
int button_mask();

typedef void (*user_gpio_callback)(BADGE_BUTTON button, bool state);
void button_set_interrupt(user_gpio_callback cb);


#endif //BADGE2022_C_BUTTON_H
