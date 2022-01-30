//
// Created by Samuel Jones on 1/29/22.
//

#include "cli_button.h"
#include "button.h"
#include <stdio.h>
#include <string.h>

static const char* _button_names[BADGE_BUTTON_MAX] = {
        "left",
        "down",
        "up",
        "right",
        "switch",
        "enc_a",
        "enc_b",
};

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

int run_button_mask(char *args) {
    int mask = button_mask();
    printf("Button mask: %02x\n", mask);
    return 0;
}


static const CLI_COMMAND button_subcommands[] = {
        {.name="get", .process=run_button_get,
                .help="usage: button get [left down up right switch enc_a enc_b]"},
        {.name="mask", .process=run_button_mask,
                .help="usage: button mask"},
        {},
};

const CLI_COMMAND button_command = {
        .name="button", .subcommands=(CLI_COMMAND *)button_subcommands,
        .help="usage: button subcommand [[args...]]\n"
              "valid subcommands: get mask"
};