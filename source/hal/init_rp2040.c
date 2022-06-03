#include <sys/cdefs.h>
//
// Created by Samuel Jones on 11/9/21.
//

#include "pico/multicore.h"
#include "pico/stdio.h"
#include "hardware/watchdog.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"
#include "pinout_rp2040.h"
#include "display_s6b33.h"
#include "led_pwm.h"
#include "button.h"
#include "ir.h"
#include "rtc.h"
#include "audio.h"

_Noreturn void core1_procedure(void) {
    // For now, nothing for core 1 to do except allow lockout and sleep forever.
    multicore_lockout_victim_init();

    while(1) {
        sleep_ms(100000);
    }
}

static void _init_gpios(void) {

    S6B33_init_gpio();
    led_pwm_init_gpio();
    button_init_gpio();
    audio_init_gpio();
}

void hal_init(void) {

    stdio_init_all();
    _init_gpios();

    ir_init();
    S6B33_reset();
    rtc_init_badge(0);
    audio_init();

    // allow suspend from other core, if we have it run something that needs to do that
    multicore_lockout_victim_init();
    multicore_launch_core1(core1_procedure);

}

int hal_run_main(int (*main_func)(int, char**), int argc, char** argv) {
    return main_func(argc, argv);
}

void hal_deinit(void) {
    multicore_reset_core1();
}

void hal_reboot(void) {
    // Go back to bootloader.
    watchdog_reboot(0, SRAM_END, 10);
}

/// disable / restore interrupt state;
uint32_t hal_disable_interrupts(void) {
    return save_and_disable_interrupts();
}

void hal_restore_interrupts(uint32_t state) {
    restore_interrupts(state);
}
