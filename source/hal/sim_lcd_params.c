#include "sim_lcd_params.h"
#include "framebuffer.h"
#include <stdio.h>

static const struct sim_lcd_params initial_default_sim_lcd_params = {
	.orientation = SIM_LCD_ORIENTATION_PORTRAIT,
	.xoffset = 1,
	.yoffset = 1,
	.width = LCD_XSIZE * 6,
	.height = LCD_YSIZE * 6,
};

static struct sim_lcd_params default_sim_lcd_params = initial_default_sim_lcd_params;
static struct sim_lcd_params sim_lcd_params = initial_default_sim_lcd_params;

void set_sim_lcd_params(struct sim_lcd_params *params)
{
	sim_lcd_params = *params;
}

struct sim_lcd_params get_sim_lcd_params(void)
{
	return sim_lcd_params;
}

void set_sim_lcd_params_default(void)
{
	sim_lcd_params = default_sim_lcd_params;
}

void adjust_sim_lcd_params_defaults(int sdl_window_width, int sdl_window_height)
{
	if (default_sim_lcd_params.width < sdl_window_width) {
		int extra = sdl_window_width - default_sim_lcd_params.width;
		default_sim_lcd_params.xoffset = 75 * (extra / 2) / 100;
	}

	if (default_sim_lcd_params.height < sdl_window_height) {
		int extra = sdl_window_height - default_sim_lcd_params.height;
		default_sim_lcd_params.yoffset = extra / 2;
	}
}

