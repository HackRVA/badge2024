//
// Created by Samuel Jones on 2/21/22.
//

#include "led_pwm.h"
#include "led_pwm_sdl.h"
#include <stdio.h>

int lcd_brightness = 255;
struct led_pwm_sdl_color led_color = { 0, 0, 0, 0xff, };
static uint8_t scale = 255;

void led_pwm_init_gpio() {

}

void led_pwm_enable(BADGE_LED led, uint8_t duty) {
    switch(led) {
        case BADGE_LED_RGB_BLUE:
            // Max RGB values are 8-bit in the simulator
            led_color.blue = (duty * scale) / 255;
            break;
        case BADGE_LED_RGB_GREEN:
            led_color.green = (duty * scale) / 255;
            break;
        case BADGE_LED_RGB_RED:
            led_color.red = (duty * scale) / 255;
            break;
        case BADGE_LED_DISPLAY_BACKLIGHT:
            lcd_brightness = duty;
            break;
        default:
            break;
    }
}

void led_pwm_disable(BADGE_LED led) {
    switch(led) {
        case BADGE_LED_RGB_BLUE:
            led_color.blue = 0;
            break;
        case BADGE_LED_RGB_GREEN:
            led_color.green = 0;
            break;
        case BADGE_LED_RGB_RED:
            led_color.red = 0;
            break;
        case BADGE_LED_DISPLAY_BACKLIGHT:
            lcd_brightness = 0;
            break;
        default:
            break;
    }
}


void led_pwm_set_scale(uint8_t new_scale) {
    scale = new_scale;
}
