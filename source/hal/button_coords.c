#include "sim_lcd_params.h"
#include "button_coords.h"

/* "Landscape" and "Portrait" here refer to the LCD screen, not the badge as a whole */
#define PORTRAITXY(x, y) { ((float) (x) / 1024.0f), ((float) (y) / 1527.0f) }
#define LANDSCAPEXY(x, y) { ((float) (x) / 1478.0f), ((float) (y) / 1024.0f) }
static const struct button_coord_list portrait_button_coords = {
	.a_button = PORTRAITXY(758, 1130),
	.b_button = PORTRAITXY(923, 1089),
#if BADGE_HAS_ROTARY_SWITCHES
	.left_rotary = PORTRAITXY(112, 96),
	.right_rotary = PORTRAITXY(960, 88),
#endif
	.dpad_up = PORTRAITXY(166, 1015),
	.dpad_down = PORTRAITXY(166, 1235),
	.dpad_left = PORTRAITXY(45, 1125),
	.dpad_right = PORTRAITXY(295, 1125),
	.led = PORTRAITXY(648, 202),
};

static const struct button_coord_list landscape_button_coords = {
	.a_button = LANDSCAPEXY(1130, 262),
	.b_button = LANDSCAPEXY(1086, 96),
#if BADGE_HAS_ROTARY_SWITCHES
	.left_rotary = LANDSCAPEXY(91, 912),
	.right_rotary = LANDSCAPEXY(84, 64),
#endif
	.dpad_right = LANDSCAPEXY(1128, 734),
	.dpad_left = LANDSCAPEXY(1128, 964),
	.dpad_up = LANDSCAPEXY(1004, 856),
	.dpad_down = LANDSCAPEXY(1244, 856),
	.led = LANDSCAPEXY(198, 374),
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
#if BADGE_HAS_ROTARY_SWITCHES
	adjust_button_coords(&current_button_coords.left_rotary, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.right_rotary, bx1, by1, f, badge_image_width, badge_image_height);
#endif
	adjust_button_coords(&current_button_coords.dpad_up, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.dpad_down, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.dpad_left, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.dpad_right, bx1, by1, f, badge_image_width, badge_image_height);
	adjust_button_coords(&current_button_coords.led, bx1, by1, f, badge_image_width, badge_image_height);
	return current_button_coords;
}

