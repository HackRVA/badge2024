#include "sim_lcd_params.h"
#include "button_coords.h"

/* "Landscape" and "Portrait" here refer to the LCD screen, not the badge as a whole */
#define PORTRAITXY(x, y) { ((float) (x) / 1024.0f), ((float) (y) / 696.0f) }
#define LANDSCAPEXY(x, y) { ((float) (x) / 696.0f), ((float) (y) / 1024.0f) }
static const struct button_coord_list portrait_button_coords = {
	.a_button = PORTRAITXY(830, 584),
	.b_button = PORTRAITXY(925, 525),
	.left_rotary = PORTRAITXY(132, 90),
	.right_rotary = PORTRAITXY(870, 87),
	.dpad_up = PORTRAITXY(140, 460),
	.dpad_down = PORTRAITXY(140, 650),
	.dpad_left = PORTRAITXY(45, 555),
	.dpad_right = PORTRAITXY(235, 555),
	.led = PORTRAITXY(809, 331),
};

static const struct button_coord_list landscape_button_coords = {
	.a_button = LANDSCAPEXY(630, 176),
	.b_button = LANDSCAPEXY(565, 70),
	.left_rotary = LANDSCAPEXY(89, 940),
	.right_rotary = LANDSCAPEXY(86, 130),
	.dpad_right = LANDSCAPEXY(600, 830),
	.dpad_left = LANDSCAPEXY(600, 1030),
	.dpad_up = LANDSCAPEXY(500, 930),
	.dpad_down = LANDSCAPEXY(700, 930),
	.led = LANDSCAPEXY(360, 200),
};

static struct button_coord_list current_button_coords;

static void adjust_button_coords(struct button_coord *b, int bx1, int by1, float f, int w, int h)
{
	b->x = (int) ((float) bx1 + f * (float) b->x * w);
	b->y = (int) ((float) by1 + f * (float) b->y * h);
}

struct button_coord_list get_button_coords(struct sim_lcd_params *slp, int badge_image_width, int badge_image_height)
{
	float f, sx1, sy1, sx2, bx1, by1;
	struct lcd_to_circuit_board_relation lcdp;


	/* Get corners of the screen inside the badge image */
	if (slp->orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
		lcdp = landscape_lcd_to_board();
	else
		lcdp = portrait_lcd_to_board();

	/* coords of sim screen on computer screen */
	sx1 = slp->xoffset;
	sy1 = slp->yoffset;
	sx2 = sx1 + slp->width;

	/* f scales from badge image coords to screen cooords */
        f = (sx2 - sx1) / (lcdp.x2 - lcdp.x1);
        bx1 =   sx1 - f * lcdp.x1;
        by1 =   sy1 - f * lcdp.y1;

	if (slp->orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
		current_button_coords = landscape_button_coords;
	else
		current_button_coords = portrait_button_coords;
	adjust_button_coords(&current_button_coords.a_button, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.b_button, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.left_rotary, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.right_rotary, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.dpad_up, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.dpad_down, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.dpad_left, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.dpad_right, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.led, bx1, by1, f, badge_image_width, badge_image_height);
	return current_button_coords;
}

