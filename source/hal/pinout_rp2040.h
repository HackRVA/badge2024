//
// Created by Samuel Jones on 12/21/21.
//

#ifndef BADGE_C_PINOUT_RP2040_H
#define BADGE_C_PINOUT_RP2040_H

#include "hardware/gpio.h"
#include "hardware/spi.h"

#define BADGE_GPIO_BTN_A (0)
#define BADGE_GPIO_BTN_B (1)

#define BADGE_GPIO_IR_RX (5)
#define BADGE_GPIO_IR_TX (6)

#define BADGE_GPIO_LED_GREEN (8) // PWM7 B
#define BADGE_GPIO_LED_RED (15) // PWM4 A
#define BADGE_GPIO_LED_BLUE (7) // PWM3 B

#define BADGE_SPI_DISPLAY (spi1)
#define BADGE_GPIO_DISPLAY_CS (9)
#define BADGE_GPIO_DISPLAY_SCK (10)
#define BADGE_GPIO_DISPLAY_MOSI (11)
#define BADGE_GPIO_DISPLAY_DC (12)
#define BADGE_GPIO_DISPLAY_RESET (13)
#define BADGE_GPIO_DISPLAY_BACKLIGHT (14) // PWM7 A

#define BADGE_I2C_COLOR_SENSOR (i2c0)
#define BADGE_I2C_COLOR_SENSOR_BAUD (100000) // 100 kHz
#define BADGE_I2C_COLOR_SENSOR_ADDR (0x38)
#define BADGE_GPIO_COLOR_SENSOR_SDA (16)
#define BADGE_GPIO_COLOR_SENSOR_SCL (17)
#define BADGE_GPIO_COLOR_SENSOR_INT (18)

#define BADGE_GPIO_AUDIO_STANDBY (19)
#define BADGE_GPIO_AUDIO_PWM (20) // PWM2 A

#define BADGE_GPIO_HALL_EFFECT_ENABLE (21)

#define BADGE_GPIO_DPAD_UP (22)
#define BADGE_GPIO_DPAD_LEFT (23)
#define BADGE_GPIO_DPAD_RIGHT (25)
#define BADGE_GPIO_DPAD_DOWN (24)

#define BADGE_GPIO_ADC_CONDUCTIVITY (26)
#define BADGE_GPIO_ADC_THERMISTOR (27)
#define BADGE_GPIO_ADC_HALL_EFFECT (28)
#define BADGE_GPIO_ADC_BATTERY (29)

#define BADGE_GPIO_MIC_SEL (2)
#define BADGE_GPIO_MIC_DAT (3)
#define BADGE_GPIO_MIC_CLK (4)

#endif //BADGE_C_PINOUT_RP2040_H
