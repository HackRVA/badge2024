#include "sim_lcd_params.h"
#include "framebuffer.h"

static const struct sim_lcd_params default_sim_lcd_params = {
	.orientation = SIM_LCD_ORIENTATION_PORTRAIT,
	.xoffset = 0,
	.yoffset = 0,
	.width = LCD_XSIZE * 6,
	.height = LCD_YSIZE * 6,
};

static struct sim_lcd_params sim_lcd_params = default_sim_lcd_params;

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

