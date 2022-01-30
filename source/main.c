#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_COMMAND_LEN 200

#include "flash_storage.h"
#include "usb.h"
#include "delay.h"
#include "init.h"

#include "cli.h"
#include "cli_flash.h"
#include "cli_led.h"
#include "cli_button.h"

#include "assets.h"
#include "framebuffer.h"
#include "display_s6b33.h"
#include "colors.h"
#include "led_pwm.h"

int exit_process(char *args) {
    return -1;
}

CLI_COMMAND exit_command = {
    .name = "exit",
    .help = "Exits command loop.",
    .process = exit_process,
};

int help_process(char *args) {
    puts("Available commands are: ");
    puts("\thelp - Print this message");
    puts("\tflash - Write data to nonvolatile storage");
    puts("\tled - Toggle lights on the device");
    puts("\tbutton - Probe button state");
    puts("\texit - Exit command loop, triggering a reboot");
    return 0;
}

CLI_COMMAND help_command = {
    .name = "help",
    .help = "Prints out the list of commands.",
    .process = help_process,
};

int main() {

    hal_init();

    // Sam: temporary code to demo display working
    led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, 150);
    S6B33_reset();
    FbMove(0,0);
    FbImage(1, 0);
    FbPushBuffer();

    // Need to ensure USB is connected before reading stdin, or else that will hang
    while (!usb_is_connected()) {
        sleep_ms(5);
    }

    CLI_COMMAND root_commands[] = {
        [0] = help_command, // Add your command name to the printout in help_process!
        [1] = exit_command,
        [2] = flash_command,
        [3] = led_command,
        [4] = button_command,
        [5] = {}
    };


    cli_run(root_commands);
    puts("Exited CLI");
    hal_deinit();
    hal_reboot();

    return 0;
}
