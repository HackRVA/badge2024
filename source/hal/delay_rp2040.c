//
// Created by Samuel Jones on 11/10/21.
//

#include "delay.h"

#include "pico/sleep.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"

#include "pico/time.h"

#include "usb.h"
#include "rtc.h"
#include "display_s6b33.h"
#include "audio.h"
#include "ir.h"
#include "button.h"
#include "led_pwm.h"


// sleep_ms function implemented by SDK (pico/time.h)
// sleep_us function implemented by SDK (pico/time.h)

void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){

    //Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;

    //reset clocks
    clocks_init();

}


void lp_sleep_us(uint64_t us_to_sleep) {

    uint64_t time_at_call = rtc_get_us_since_boot();

    if (usb_is_connected()) {
        // No need to go low power, and disabling clocks with USB connected
        // is probably a bad idea anyway
        sleep_us(us_to_sleep);
        return;
    }

    while (((S6B33_busy() || audio_is_playing() || button_debouncing() || ir_listening()) ||
             led_pwm_is_on(BADGE_LED_RGB_RED) || led_pwm_is_on(BADGE_LED_RGB_BLUE) ||
             led_pwm_is_on(BADGE_LED_RGB_GREEN)) &&
            (rtc_get_us_since_boot() < time_at_call+us_to_sleep)) {
        sleep_us(10);
    }

    uint64_t time_before_lpsleep = rtc_get_us_since_boot() - time_at_call;

    if (time_before_lpsleep >= us_to_sleep) {
        return;
    }

    uint scb_orig = scb_hw->scr;
    uint clock0_orig = clocks_hw->sleep_en0;
    uint clock1_orig = clocks_hw->sleep_en1;

    sleep_run_from_xosc();
    sleep_us(us_to_sleep-time_before_lpsleep);
    recover_from_sleep(scb_orig, clock0_orig, clock1_orig);

}