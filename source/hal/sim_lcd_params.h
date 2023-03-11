#ifndef SIM_LCD_PARAMS_
#define SIM_LCD_PARAMS_

struct sim_lcd_params {
	int orientation; /* LCD screen orientation (not orientation of badge as a whole) */
			/* When LCD is portrait, badge as a whole is landscape, and vice versa, since 2023 */
#define SIM_LCD_ORIENTATION_PORTRAIT 0
#define SIM_LCD_ORIENTATION_LANDSCAPE 1
	int xoffset, yoffset; /* offset into the SDL window */
	int width, height; /* width and height in the SDL window */
};

void set_sim_lcd_params(struct sim_lcd_params *params);
void set_sim_lcd_params_default(void);
struct sim_lcd_params get_sim_lcd_params(void);
void adjust_sim_lcd_params_defaults(int sdl_window_width, int sdl_window_height);
void set_sim_lcd_params_landscape(void);
void set_sim_lcd_params_portrait(void);

#endif

