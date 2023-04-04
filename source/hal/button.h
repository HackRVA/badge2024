//
// Created by Samuel Jones on 1/29/22.
//

#ifndef BADGE_C_BUTTON_H
#define BADGE_C_BUTTON_H

#include <stdbool.h>

typedef enum {
    BADGE_BUTTON_LEFT = 0,
    BADGE_BUTTON_DOWN,
    BADGE_BUTTON_UP,
    BADGE_BUTTON_RIGHT,
    BADGE_BUTTON_SW,
    BADGE_BUTTON_SW2,
    BADGE_BUTTON_ENCODER_A,
    BADGE_BUTTON_ENCODER_B,
    BADGE_BUTTON_MAX
} BADGE_BUTTON;

#define BUTTON_PRESSED(button, latches) ((latches) & (1 << (button)))

void button_init_gpio(void);

// Poll button state. Returns 1 if pressed, 0 if not pressed
int button_poll(BADGE_BUTTON button);

// Get a bitmask of buttons.
int button_mask();

// Get up and down latches. These will return the corresponding bit once each call.
int button_down_latches(void);
int button_up_latches(void);
void clear_latches(void);

typedef void (*user_gpio_callback)(BADGE_BUTTON button, bool state);
void button_set_interrupt(user_gpio_callback cb);

// TODO maybe we can track this outside of the HAL and in the badge system files somewhere
unsigned int button_last_input_timestamp(void);
void button_reset_last_input_timestamp(void);

// Get rotary encoder rotations! Rotation count is automatically cleared between calls.
// Positive indicates CW, negative indicates CCW.
int button_get_rotation(unsigned which_rotary);

/**  Tell us if we're busy debouncing a button input. */
bool button_debouncing(void);

#endif //BADGE_C_BUTTON_H
