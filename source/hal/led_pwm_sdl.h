#ifndef LED_PWM_SDL_H_
#define LED_PWM_SDL_H_
#include <stdint.h>

extern struct led_pwm_sdl_color {
	uint8_t red, green, blue, alpha;
} led_color;

#endif
