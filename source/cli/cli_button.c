//
// Created by Samuel Jones on 1/29/22.
//

#include "cli_button.h"
#include "button.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static const char* _button_names[BADGE_BUTTON_MAX] = {
        "left",
        "down",
        "up",
        "right",
        "switch",
        "enc_a",
        "enc_b",
};

static uint8_t _button_up_counts[BADGE_BUTTON_MAX];
static uint8_t _button_down_counts[BADGE_BUTTON_MAX];

static BADGE_BUTTON _button_for_name(const char* name) {
    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        if (strcmp(_button_names[i], name) == 0) {
            return i;
        }
    }

    return BADGE_BUTTON_MAX;
}


int run_button_get(char *args) {

    char * button_name = cli_get_token(&args);
    if (!button_name) {
        puts("No button specified.");
        return 1;
    }

    BADGE_BUTTON button = _button_for_name(button_name);
    if (button >= BADGE_BUTTON_MAX) {
        puts("Invalid button name specified.");
        return 1;
    }

    int pressed = button_poll(button);
    if (pressed) {
        puts("Button is pressed.");
    } else {
        puts("Button is not pressed.");
    }

    return 0;
}

int run_button_mask(__attribute__((unused)) char *args) {
    int mask = button_mask();
    printf("Button mask: %02x\n", mask);
    return 0;
}


void _button_event_handler(BADGE_BUTTON button, bool high) {
    if (high) {
        _button_up_counts[button] += 1;
    } else {
        _button_down_counts[button] += 1;
    }
}

int run_install_handler(__attribute__((unused)) char* args) {
    button_set_interrupt(_button_event_handler);
    printf("Installed button event handler to count events.\n");
    return 0;
}

int run_get_event_counts(__attribute__((unused)) char*args) {
    for (int i=0; i<BADGE_BUTTON_MAX; i++) {
        printf("Button %s - down %u times, up %u times.\n",
               _button_names[i], _button_down_counts[i], _button_up_counts[i]);
    }
    return 0;
}


static const CLI_COMMAND button_subcommands[] = {
        {.name="get", .process=run_button_get,
                .help="usage: button get [left down up right switch enc_a enc_b]"},
        {.name="mask", .process=run_button_mask,
                .help="usage: button mask"},
        {.name="handler", .process=run_install_handler,
                .help="usage: button handler - Install event counting button IRQ handler"},
        {.name="events", .process=run_get_event_counts,
                .help="usage: button events - Show cumulative button event counts after handler install"},
        {},
};

const CLI_COMMAND button_command = {
        .name="button", .subcommands=(CLI_COMMAND *)button_subcommands,
        .help="usage: button subcommand [[args...]]\n"
              "valid subcommands: get mask handler events"
};
