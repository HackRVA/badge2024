#ifndef BUTTON_COORDS_H_
#define BUTTON_COORDS_H_

#include "sim_lcd_params.h"

struct button_coord {
		float x, y;
};

struct button_coord_list {
	struct button_coord a_button;
	struct button_coord b_button;
	struct button_coord left_rotary, right_rotary;
	struct button_coord dpad_up;
	struct button_coord dpad_down;
	struct button_coord dpad_left;
	struct button_coord dpad_right;
	struct button_coord led;
};

struct button_coord_list get_button_coords(struct sim_lcd_params *slp, int badge_image_width, int badge_image_height);

#endif

