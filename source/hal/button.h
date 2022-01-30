//
// Created by Samuel Jones on 1/29/22.
//

#ifndef BADGE2022_C_BUTTON_H
#define BADGE2022_C_BUTTON_H

typedef enum {
    BADGE_BUTTON_LEFT = 0,
    BADGE_BUTTON_DOWN,
    BADGE_BUTTON_UP,
    BADGE_BUTTON_RIGHT,
    BADGE_BUTTON_ROTARY_SW,
    BADGE_BUTTON_MAX
} BADGE_BUTTON;


void button_init_gpio(void);

// Poll button state. Returns 1 if pressed, 0 if not pressed
int button_poll(BADGE_BUTTON button);
int button_mask();


#endif //BADGE2022_C_BUTTON_H
