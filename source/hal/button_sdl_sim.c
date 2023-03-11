//
// Created by Samuel Jones on 2/21/22.
//
#include <SDL.h>
#include "button.h"
#include "rtc.h"

#define UNUSED __attribute__((unused))

int time_to_quit = 0;
static int down_latches = 0;
static int up_latches = 0;
static int button_states = 0;
static int rotation_count = 0;
static uint64_t last_change = 0;
static user_gpio_callback callback = NULL;

int key_press_cb(SDL_Keysym *keysym)
{
    BADGE_BUTTON button = BADGE_BUTTON_MAX;
    switch (keysym->sym) {
        case SDLK_w:
        case SDLK_UP:
            button = BADGE_BUTTON_UP;
        break;
        case SDLK_s:
        case SDLK_DOWN:
            button = BADGE_BUTTON_DOWN;
        break;
        case SDLK_a:
        case SDLK_LEFT:
            button = BADGE_BUTTON_LEFT;
        break;
        case SDLK_d:
        case SDLK_RIGHT:
            button = BADGE_BUTTON_RIGHT;
        break;
        case SDLK_SPACE:
        case SDLK_RETURN:
            button = BADGE_BUTTON_SW;
        break;
        case SDLK_q:
        case SDLK_ESCAPE:
            time_to_quit = 1;
        break;
        case SDLK_COMMA:
        case SDLK_LESS:
            rotation_count -= 1;
        break;
        case SDLK_PERIOD:
        case SDLK_GREATER:
            rotation_count += 1;
        break;
        default:
            break;
    }
    if (button != BADGE_BUTTON_MAX) {
        down_latches |= 1<<button;
        button_states |= 1<<button;
        if (callback) {
            callback(button, true);
        }
        last_change = rtc_get_ms_since_boot();
    }
    return 1;
}

int key_release_cb(SDL_Keysym *keysym)
{
    BADGE_BUTTON button = BADGE_BUTTON_MAX;
    switch (keysym->sym) {
        case SDLK_w:
        case SDLK_UP:
            button = BADGE_BUTTON_UP;
            break;
        case SDLK_s:
        case SDLK_DOWN:
            button = BADGE_BUTTON_DOWN;
            break;
        case SDLK_a:
        case SDLK_LEFT:
            button = BADGE_BUTTON_LEFT;
            break;
        case SDLK_d:
        case SDLK_RIGHT:
            button = BADGE_BUTTON_RIGHT;
            break;
        case SDLK_SPACE:
        case SDLK_RETURN:
            button = BADGE_BUTTON_SW;
            break;
        case SDLK_q:
        case SDLK_ESCAPE:
            time_to_quit = 1;
        default:
            break;
    }
    if (button != BADGE_BUTTON_MAX) {
        up_latches |= 1<<button;
        button_states &= ~(1<<button);
        if (callback) {
            callback(button, false);
        }
        last_change = rtc_get_ms_since_boot();
    }
    return 1;
}


void button_init_gpio(void) {
    last_change = rtc_get_ms_since_boot();
}

// Poll button state. Returns 1 if pressed, 0 if not pressed
int button_poll(BADGE_BUTTON button) {
    return button_states & (1<<button) ? 1 : 0;
}

// Get a bitmask of buttons.
int button_mask() {
    return button_states;
}

int button_down_latches(void) {
    int return_val = down_latches;
    down_latches = 0;
    return return_val;
}

int button_up_latches(void) {
    int return_val = up_latches;
    up_latches = 0;
    return return_val;
}

void clear_latches(void) {
    down_latches = 0;
    up_latches = 0;
}

typedef void (*user_gpio_callback)(BADGE_BUTTON button, bool state);
void button_set_interrupt(user_gpio_callback cb) {
    callback = cb;
}

// TODO maybe we can track this outside of the HAL and in the badge system files somewhere
unsigned int button_last_input_timestamp(void) {
    return last_change;
}

void button_reset_last_input_timestamp(void) {
    last_change = rtc_get_ms_since_boot();
}

/* TODO: make this work with multiple rotary switches */
int button_get_rotation(__attribute__((unused)) int which_rotary) {
    int count = rotation_count;
    rotation_count = 0;
    return count;
}
