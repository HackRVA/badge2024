//
// Created by Samuel Jones on 12/27/21.
//

#include "cli_led.h"
#include "led_pwm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char* _led_names[BADGE_LED_MAX] = {
        "red",
        "green",
        "blue",
        "backlight",
};

static BADGE_LED _led_for_name(const char* name) {
    for (int i=0; i<BADGE_LED_MAX; i++) {
        if (strcmp(_led_names[i], name) == 0) {
            return i;
        }
    }

    return BADGE_LED_MAX;
}

int run_led_on(char *args) {

    char * led_name = cli_get_token(&args);
    if (!led_name) {
        puts("No LED specified.");
        return 1;
    }

    BADGE_LED led = _led_for_name(led_name);
    if (led >= BADGE_LED_MAX) {
        puts("Invalid LED name specified.");
        return 1;
    }

    // Default 100% if nothing specified
    int duty = 255;
    char * duty_string = cli_get_token(&args);
    if (duty_string) {
        duty = strtol(duty_string, NULL, 10);
    }
    if (duty >= 256 || duty < 0) {
        puts("Invalid duty level specified (must be 0-255");
        return 1;
    }

    printf("Enabling LED output (%s at %d/255 duty cycle)...\n", led_name, duty);
    led_pwm_enable(led, duty);
    return 0;
}

int run_led_off(char *args) {

    char * led_name = cli_get_token(&args);
    if (!led_name) {
        puts("No LED specified.");
        return 1;
    }

    BADGE_LED led = _led_for_name(led_name);
    if (led >= BADGE_LED_MAX) {
        puts("Invalid LED name specified.");
        return 1;
    }

    printf("Disabling LED output (%s)...\n", led_name);
    led_pwm_disable(led);
    return 0;
}

static const CLI_COMMAND led_subcommands[] = {
        {.name="on", .process=run_led_on,
         .help="usage: led on [red green blue white backlight] [[level, 0-255]]"},
        {.name="off", .process=run_led_off,
        .help="usage: led off [red green blue white backlight]"},
        {}
};


const CLI_COMMAND led_command = {
        .name="led", .subcommands=(CLI_COMMAND *) led_subcommands,
        .help="usage: led subcommand [[args...]]\n"
              "valid subcommands: on off"
};