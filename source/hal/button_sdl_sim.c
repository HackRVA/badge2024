//
// Created by Samuel Jones on 2/21/22.
//

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

int key_press_cb()
{
#if 0
    BADGE_BUTTON button = BADGE_BUTTON_MAX;
    switch (event->keyval) {
        case GDK_w:
        case GDK_KEY_Up:
            button = BADGE_BUTTON_UP;
        break;
        case GDK_s:
        case GDK_KEY_Down:
            button = BADGE_BUTTON_DOWN;
        break;
        case GDK_a:
        case GDK_KEY_Left:
            button = BADGE_BUTTON_LEFT;
        break;
        case GDK_d:
        case GDK_KEY_Right:
            button = BADGE_BUTTON_RIGHT;
        break;
        case GDK_space:
        case GDK_KEY_Return:
            button = BADGE_BUTTON_SW;
        break;
        case GDK_q:
        case GDK_KEY_Escape:
            time_to_quit = 1;
        break;
        case GDK_comma:
        case GDK_less:
            rotation_count -= 1;
        break;
        case GDK_period:
        case GDK_greater:
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
    return TRUE;
#endif
}


int key_release_cb()
{
#if 0

    BADGE_BUTTON button = BADGE_BUTTON_MAX;
    switch (event->keyval) {
        case GDK_w:
        case GDK_KEY_Up:
            button = BADGE_BUTTON_UP;
            break;
        case GDK_s:
        case GDK_KEY_Down:
            button = BADGE_BUTTON_DOWN;
            break;
        case GDK_a:
        case GDK_KEY_Left:
            button = BADGE_BUTTON_LEFT;
            break;
        case GDK_d:
        case GDK_KEY_Right:
            button = BADGE_BUTTON_RIGHT;
            break;
        case GDK_space:
        case GDK_KEY_Return:
            button = BADGE_BUTTON_SW;
            break;
        case GDK_q:
        case GDK_KEY_Escape:
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
    return TRUE;
#endif
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

int button_get_rotation(void) {
    int count = rotation_count;
    rotation_count = 0;
    return count;
}
