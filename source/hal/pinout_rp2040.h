//
// Created by Samuel Jones on 12/21/21.
//

#ifndef BADGE_C_PINOUT_RP2040_H
#define BADGE_C_PINOUT_RP2040_H

#include "hardware/gpio.h"

#define BADGE_GPIO_DISPLAY_CS (17)
#define BADGE_GPIO_DISPLAY_SCK (18)
#define BADGE_GPIO_DISPLAY_MOSI (19)
#define BADGE_GPIO_DISPLAY_RESET (21)
#define BADGE_GPIO_DISPLAY_DC (20)
#define BADGE_GPIO_DISPLAY_BACKLIGHT (22) // PWM controlled

#define BADGE_GPIO_DPAD_UP (2)
#define BADGE_GPIO_DPAD_RIGHT (3)
#define BADGE_GPIO_DPAD_DOWN (1)
#define BADGE_GPIO_DPAD_LEFT (0)

// Pins on rotary encoder
#define BADGE_GPIO_SW (12)
#define BADGE_GPIO_ENCODER_A  (15)
#define BADGE_GPIO_ENCODER_B  (13)

#define BADGE_GPIO_IR_TX (5)
#define BADGE_GPIO_IR_RX (4)

#define BADGE_GPIO_AUDIO_STANDBY (26)
#define BADGE_GPIO_AUDIO_PWM (27)
#define BADGE_GPIO_AUDIO_INPUT (28)

#define BADGE_GPIO_LED_GREEN (9)
#define BADGE_GPIO_LED_RED (8)
#define BADGE_GPIO_LED_BLUE (7)


#endif //BADGE_C_PINOUT_RP2040_H
