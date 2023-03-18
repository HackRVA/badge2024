//
// Created by Samuel Jones on 12/21/21.
//

#ifndef BADGE_C_DISPLAY_H
#define BADGE_C_DISPLAY_H

#include <stdbool.h>

/* display mode settings for conditionals */
#define DISPLAY_MODE_NORMAL 0b00000110
#define DISPLAY_MODE_INVERTED 0b00000011

/** set important internal registers for the LCD display */
void display_init_device(void);
/** set GPIO configuration for the LCD display */
void display_init_gpio(void);
/** Perform init sequence on display */
void display_reset(void);
/** sets the current display region for calls to display_pixel() */
void display_rect(int x, int y, int width, int height);
/** updates current pixel to the data in `pixel`. */
void display_pixel(unsigned short pixel);
/** Updates a consecutive sequence of pixels. */
void display_pixels(unsigned short *pixel, int number);
/** invert display */
void display_set_display_mode_inverted(void);
/** uninvert display */
void display_set_display_mode_noninverted(void);
/** get current display mode variable setting */
unsigned char display_get_display_mode(void);

int display_get_rotation(void);
void display_set_rotation(int yes);
void display_color(unsigned short pixel);

/** @brief Tell us if we're busy sending data to the display */
bool display_busy(void);

#endif //BADGE_C_DISPLAY_H
