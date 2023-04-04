//
// Created by Samuel Jones on 12/21/21.
//

#ifndef BADGE_C_PINOUT_RP2040_H
#define BADGE_C_PINOUT_RP2040_H

#include "hardware/gpio.h"
#include "hardware/spi.h"

#define BADGE_GPIO_BTN_A (0)
#define BADGE_GPIO_BTN_B (1)

#define BADGE_GPIO_IR_RX (2)
#define BADGE_GPIO_IR_TX (8)

#define BADGE_SPI_ACCEL (spi0)
#define BADGE_GPIO_ACCEL_CS (5)
#define BADGE_GPIO_ACCEL_MOSI (3)
#define BADGE_GPIO_ACCEL_MISO (4)
#define BADGE_GPIO_ACCEL_SCK (6)
#define BADGE_GPIO_ACCEL_INT1 (7)

#define BADGE_SPI_DISPLAY (spi1)
#define BADGE_GPIO_DISPLAY_CS (9)
#define BADGE_GPIO_DISPLAY_SCK (10)
#define BADGE_GPIO_DISPLAY_MOSI (11)
#define BADGE_GPIO_DISPLAY_DC (12)
#define BADGE_GPIO_DISPLAY_RESET (13)
#define BADGE_GPIO_DISPLAY_BACKLIGHT (14) // PWM controlled

#define BADGE_GPIO_ENCODER_A  (16)
#define BADGE_GPIO_ENCODER_B  (17)
#define BADGE_GPIO_SW (18)

#define BADGE_GPIO_2_SW (19)
#define BADGE_GPIO_ENCODER_2_A  (20)
#define BADGE_GPIO_ENCODER_2_B  (21)

#define BADGE_GPIO_DPAD_UP (22)
#define BADGE_GPIO_DPAD_LEFT (23)
#define BADGE_GPIO_DPAD_RIGHT (24)
#define BADGE_GPIO_DPAD_DOWN (25)

#define BADGE_GPIO_AUDIO_STANDBY (26)
#define BADGE_GPIO_AUDIO_PWM (27)
#define BADGE_GPIO_AUDIO_INPUT (28)

#define BADGE_GPIO_BATTERY (29)

/* These are all unused and unsupported - put on unused pin for now -PMW */
#define BADGE_GPIO_LED_GREEN (15)
#define BADGE_GPIO_LED_RED (15)
#define BADGE_GPIO_LED_BLUE (15)

#endif //BADGE_C_PINOUT_RP2040_H
