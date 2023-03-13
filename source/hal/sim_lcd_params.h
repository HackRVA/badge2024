#ifndef SIM_LCD_PARAMS_
#define SIM_LCD_PARAMS_

struct sim_lcd_params {
	int orientation;
#define SIM_LCD_ORIENTATION_PORTRAIT 0
#define SIM_LCD_ORIENTATION_LANDSCAPE 1
	int xoffset, yoffset; /* offset into the SDL window */
	int width, height; /* width and height in the SDL window */
};

void set_sim_lcd_params(struct sim_lcd_params *params);
void set_sim_lcd_params_default(void);
struct sim_lcd_params get_sim_lcd_params(void);

#endif

