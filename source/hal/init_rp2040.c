#include <sys/cdefs.h>
//
// Created by Samuel Jones on 11/9/21.
//

#include "pico/multicore.h"
#include "pico/stdio.h"
#include "hardware/watchdog.h"
#include "pinout_rp2040.h"
#include "display_s6b33.h"
#include "led_pwm.h"
#include "button.h"


_Noreturn void core1_procedure(void) {
    // For now, nothing for core 1 to do except allow lockout and sleep forever.
    multicore_lockout_victim_init();

    while(1) {
        busy_wait_ms(1000);
    }
}

static void _init_gpios(void) {

    S6B33_init_gpio();
    led_pwm_init_gpio();
    button_init_gpio();

    gpio_init(BADGE_GPIO_IR_TX);
    gpio_init(BADGE_GPIO_IR_RX);

    gpio_init(BADGE_GPIO_AUDIO_INPUT);
    gpio_init(BADGE_GPIO_AUDIO_PWM);
    gpio_init(BADGE_GPIO_AUDIO_STANDBY);

    // audio standby should be always driven, start off
    gpio_set_dir(BADGE_GPIO_AUDIO_STANDBY, true);
    gpio_put(BADGE_GPIO_AUDIO_STANDBY, 0);
}

void hal_init(void) {

    stdio_init_all();
    _init_gpios();

    // allow suspend from other core, if we have it run something that needs to do that
    multicore_lockout_victim_init();
    multicore_launch_core1(core1_procedure);

}

void hal_deinit(void) {
    multicore_reset_core1();
}

void hal_reboot(void) {
    // Go back to bootloader.
    watchdog_reboot(0, SRAM_END, 10);
}