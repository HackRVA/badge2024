#include <sys/cdefs.h>
//
// Created by Samuel Jones on 11/9/21.
//

#include "pico/multicore.h"
#include "pico/stdio.h"
#include "hardware/watchdog.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"
#include "hardware/exception.h"
#include "pinout_rp2040.h"
#include "display.h"
#include "led_pwm.h"
#include "button.h"
#include "ir.h"
#include "rtc.h"
#include "audio.h"
#include "accelerometer.h"

#include <framebuffer.h>
#include <colors.h>
#include <utils.h>
#include <stdio.h>

static const char *const stacked_reg[] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r12",
    "lr",
    "pc",
    "psr",
};

void show_reg(const char *const name, uint32_t reg)
{
    char msg[16] = {0};
    snprintf(msg, sizeof(msg), "%3s:%08x\n", name, reg);
    FbWriteString(msg);
}

void dump_fault(uint32_t *sp)
{

    FbClear();
    FbMove(0,0);
    FbColor(RED);
    FbWriteString("HARD FAULT\n");

    for (size_t i = 0; i < ARRAY_SIZE(stacked_reg); i++) {
        show_reg(stacked_reg[i], sp[i]);
    }

    show_reg("sp", (uint32_t) sp);

    FbSwapBuffers();

    /* Mute audio */
    audio_init();

    while(1);
}

__attribute__((naked)) void hard_fault_handler(void)
{
    __asm volatile (
        "   mov r0, lr          \n"
        "   ldr r1, mask_lr     \n"
        "   tst r0, r1          \n"
        "   bne stack_psp       \n"
        "   mrs r0, msp         \n"
        "   b b_dump_fault      \n"
        "stack_psp:             \n"
        "   mrs r0, psp         \n"
        "b_dump_fault:          \n"
        "   b dump_fault        \n"
        "mask_lr: .word 0x00000004 \n"
    );
}

_Noreturn void core1_procedure(void) {
    // For now, nothing for core 1 to do except allow lockout and sleep forever.
    multicore_lockout_victim_init();

    while(1) {
        sleep_ms(100000);
    }
}

static void _init_gpios(void) {

    display_init_gpio();
    led_pwm_init_gpio();
    button_init_gpio();
    audio_init_gpio();
    accelerometer_init_gpio();
}

void hal_init(void) {

    stdio_init_all();
    _init_gpios();

    ir_init();
    display_reset();
    rtc_init_badge(0);
    audio_init();
    accelerometer_init();

    exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hard_fault_handler);

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
