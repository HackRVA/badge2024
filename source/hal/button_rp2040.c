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

#define DEBOUNCE_DELAY_MS 3

static const int8_t button_gpios[BADGE_BUTTON_MAX] = {
    BADGE_GPIO_DPAD_LEFT,
    BADGE_GPIO_DPAD_DOWN,
    BADGE_GPIO_DPAD_UP,
    BADGE_GPIO_DPAD_RIGHT,
    // Rotary encoder button
    BADGE_GPIO_SW,
    // Rotary encoder state
    BADGE_GPIO_ENCODER_A,
    BADGE_GPIO_ENCODER_B,
};

static critical_section_t critical_section;
static volatile uint8_t debounce_alarm_count;

// bitmask of reported debounced pin states
static uint32_t gpio_states;
static uint32_t down_latches;
static uint32_t up_latches;
static uint32_t last_change;
static int rotation_count;

// callback
static user_gpio_callback user_cb;

// forward declaration of GPIO callback since the alarm and GPIO handlers need to refer to each other
static void gpio_callback(uint gpio, uint32_t events);

static void process_rotary_pin_state(uint gpio, int state) {
    if (gpio == BADGE_BUTTON_ENCODER_A && state == 0) {
        rotation_count += button_poll(BADGE_BUTTON_ENCODER_B) ? 1 : -1;
    }
}

int64_t alarm_callback(__attribute__((unused)) alarm_id_t id, void* user_data) {

    uint gpio_enum = (uint)user_data;
    uint gpio = button_gpios[gpio_enum];

    int state = gpio_get(gpio) ? 1 : 0;
    int prev_state = (gpio_states & (1u<<gpio_enum)) ? 1 : 0;

    if (state != prev_state) {
        gpio_states &= ~(1<<gpio_enum);
        gpio_states |= state<<gpio_enum;

        if (state) {
            up_latches |= 1<<gpio_enum;
        } else {
            down_latches |= 1<<gpio_enum;
        }

        if (user_cb) {
            user_cb(gpio_enum, state);
        }
        process_rotary_pin_state(gpio_enum, state);
        last_change = rtc_get_ms_since_boot();
    }

    if (debounce_alarm_count) {
        debounce_alarm_count--;
    }

    gpio_set_irq_enabled_with_callback(gpio,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true,
                                       gpio_callback);
    return 0;
}

void gpio_callback(uint gpio, __attribute__((unused)) uint32_t events) {
    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        uint gpio_pin = (uint)(button_gpios[i]);
        if (gpio_pin == gpio) {
            gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
            // After time delay, check for bounce being done.
            debounce_alarm_count++;
            alarm_pool_add_alarm_in_ms(alarm_pool_get_default(),
                                       DEBOUNCE_DELAY_MS,
                                       alarm_callback,
                                       (void*)i,
                                       true);
        }
    }
}

void button_init_gpio(void) {

    alarm_pool_init_default();
    critical_section_init(&critical_section);

    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        uint gpio = (uint) button_gpios[i];
        gpio_init(gpio);
        gpio_set_input_enabled(gpio, 1);
        gpio_set_input_hysteresis_enabled(gpio, 1);
        gpio_pull_up(gpio);
    }

    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        uint gpio = (uint) button_gpios[i];
        uint state = gpio_get(button_gpios[i]) ? 1 : 0;
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

/* TODO: make this work with multiple rotary switches */
int button_get_rotation(__attribute__((unused)) int which_rotary) {
    critical_section_enter_blocking(&critical_section);
    int count = rotation_count;
    rotation_count = 0;
    critical_section_exit(&critical_section);
    return count;
}


bool button_debouncing(void) {
    return (bool) debounce_alarm_count;
}
