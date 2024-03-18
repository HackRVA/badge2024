//
// Created by Samuel Jones on 1/29/22.
//

#include "button.h"
#include "pinout_rp2040.h"
#include "pico/time.h"
#include "pico/sync.h"
#include <stdint.h>
#include <stdio.h>
#include "rtc.h"
#include <utils.h>

#define DEBOUNCE_DELAY_MS 3

/* Map enum BADGE_BUTTON_... to BADGE_GPIO_... (badge buttons to gpio pins) */
static const int8_t button_to_gpio_pin[BADGE_BUTTON_MAX] = {
    /* A/B buttons */    
    BADGE_GPIO_BTN_A,
    BADGE_GPIO_BTN_B,

    /* D-Pad */
    BADGE_GPIO_DPAD_LEFT,
    BADGE_GPIO_DPAD_DOWN,
    BADGE_GPIO_DPAD_UP,
    BADGE_GPIO_DPAD_RIGHT,

#if BADGE_HAS_ROTARY_SWITCHES
    /* Rotary encoder 1 */
    BADGE_GPIO_SW,
    BADGE_GPIO_ENCODER_A,
    BADGE_GPIO_ENCODER_B,
    
    /* Rotary encoder 2 */
    BADGE_GPIO_2_SW,
    BADGE_GPIO_ENCODER_2_A,
    BADGE_GPIO_ENCODER_2_B,
#endif

};

static critical_section_t critical_section;
static volatile uint8_t debounce_alarm_count;

// bitmask of reported debounced pin states
static uint32_t gpio_states;
static uint32_t down_latches;
static uint32_t up_latches;
static uint32_t last_change;
static int rotation_count[2];

// callback
static user_gpio_callback user_cb;

// forward declaration of GPIO callback since the alarm and GPIO handlers need to refer to each other
static void gpio_callback(uint gpio_pin, uint32_t events);

#if BADGE_HAS_ROTARY_SWITCHES
static void process_rotary_pin_state(uint badge_button, int state) {
    int idx;
    int b;

    if (badge_button == BADGE_BUTTON_ENCODER_A) {
        idx = 0;
        b = BADGE_BUTTON_ENCODER_B;
    } else if (badge_button == BADGE_BUTTON_ENCODER_2_A) {
        idx = 1;
        b = BADGE_BUTTON_ENCODER_2_B;
    } else {
        return;
    }

    if (state == 0) {
        rotation_count[idx] += button_poll(b) ? 1 : -1;
    }
}
#endif

int64_t alarm_callback(__attribute__((unused)) alarm_id_t id, void* user_data) {

    uint badge_button = (uint) user_data;
    uint gpio_pin = button_to_gpio_pin[badge_button];

    int state = gpio_get(gpio_pin) ? 1 : 0;
    int prev_state = (gpio_states & (1u<<badge_button)) ? 1 : 0;

    if (state != prev_state) {
        gpio_states &= ~(1<<badge_button);
        gpio_states |= state<<badge_button;

        if (state) {
            up_latches |= 1<<badge_button;
        } else {
            down_latches |= 1<<badge_button;
        }

        if (user_cb) {
            user_cb(badge_button, state);
        }
#if BADGE_HAS_ROTARY_SWITCHES
        process_rotary_pin_state(badge_button, state);
#endif
        last_change = rtc_get_ms_since_boot();
    }

    if (debounce_alarm_count) {
        debounce_alarm_count--;
    }

    gpio_set_irq_enabled_with_callback(gpio_pin,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true,
                                       gpio_callback);
    return 0;
}

void gpio_callback(uint gpio, __attribute__((unused)) uint32_t events) {
    for (int button = 0; button < BADGE_BUTTON_MAX; button++) {
        uint gpio_pin = (uint)(button_to_gpio_pin[button]);
        if (gpio_pin == gpio) {
            gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
            // After time delay, check for bounce being done.
            debounce_alarm_count++;

            /* It is explicitly OK to call this here, even from interrupt context
             * and even from an alarm callback.
             * See: pico-sdk-src/src/common/pico_time/include/pico/time.h line 477
             * and surrounding comments.
             */
            alarm_pool_add_alarm_in_ms(alarm_pool_get_default(),
                                       DEBOUNCE_DELAY_MS,
                                       alarm_callback,
                                       (void*) button,
                                       true);
        }
    }
}

void button_init_gpio(void) {

    alarm_pool_init_default();
    critical_section_init(&critical_section);

    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        uint gpio = (uint) button_to_gpio_pin[i];
        gpio_init(gpio);
        gpio_set_input_enabled(gpio, 1);
        gpio_set_input_hysteresis_enabled(gpio, 1);
        gpio_pull_up(gpio);
    }

    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        uint gpio = (uint) button_to_gpio_pin[i];
        uint state = gpio_get(button_to_gpio_pin[i]) ? 1 : 0;
        gpio_states |= (state<<i);
        gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true,
                                           gpio_callback);
    }
}

int button_poll(BADGE_BUTTON button) {
    return (gpio_states & (1 << button)) ? 0 : 1;
}

int button_mask(void) {
    return (int) ((~gpio_states) & ((1<<BADGE_BUTTON_MAX)-1));
}

int button_down_latches(void) {
    critical_section_enter_blocking(&critical_section);
    int response = (int) down_latches;
    down_latches = 0;
    critical_section_exit(&critical_section);
    return response;
}

int button_up_latches(void) {
    critical_section_enter_blocking(&critical_section);
    int response = (int) up_latches;
    up_latches = 0;
    critical_section_exit(&critical_section);
    return response;
}

void clear_latches(void) {
    critical_section_enter_blocking(&critical_section);
    up_latches = 0;
    down_latches = 0;
    critical_section_exit(&critical_section);
}

void button_set_interrupt(user_gpio_callback cb) {
    user_cb = cb;
}

unsigned int button_last_input_timestamp(void) {
    return last_change;
}

void button_reset_last_input_timestamp(void) {
    last_change = rtc_get_ms_since_boot();
}

int button_get_rotation(unsigned int i) {
    if (ARRAY_SIZE(rotation_count) < i) {
        return 0;
    }

    critical_section_enter_blocking(&critical_section);

    int count = rotation_count[i];
    rotation_count[i] = 0;

    critical_section_exit(&critical_section);

    return count;
}


bool button_debouncing(void) {
    return (bool) debounce_alarm_count;
}
