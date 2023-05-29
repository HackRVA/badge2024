#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "trig.h"
#include "fxp_sqrt.h"

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Program states.  Initial state is TANK_VS_TANK_INIT */
enum tank_vs_tank_state_t {
	TANK_VS_TANK_INIT,
	TANK_VS_TANK_RUN,
	TANK_VS_TANK_EXIT,
};

static enum tank_vs_tank_state_t tank_vs_tank_state = TANK_VS_TANK_INIT;

static struct point tank_points[] = {
	{ 0, -5 },
	{ 3, 5 },
	{ -3, 5 },
	{ 0, - 5 },
};

static struct tank {
	int x, y, angle, color;
} tank[2] = { 0 };

static void tank_vs_tank_init(void)
{
	FbInit();
	FbClear();
	tank_vs_tank_state = TANK_VS_TANK_RUN;
	tank[0].color = YELLOW;
	tank[0].x = 256 * (LCD_XSIZE / 2);
	tank[0].y = 256 * 10;
	tank[0].angle = 32 * 2;
	tank[1].color = CYAN;
	tank[1].x = 256 * (LCD_XSIZE / 2);
	tank[1].y = 256 * (LCD_YSIZE - 10);
	tank[1].angle = 0;
}

static void rotate_point(const struct point *source, struct point *dest, int angle)
{
	int a;
	int len = 256 * (source->x * source ->x + source->y * source->y);
	len = fxp_sqrt(len);
	len = len / 256;

	a = arctan2(source->y, source->x);
	if (a < 0)
		a = a + 128;
	a = a + angle;
	if (a > 127)
		a -= 128;
	if (a < 0)
		a += 128;
	dest->x = len * cosine(a) / 256;
	dest->y = len * sine(a) / 256;
}

static void rotate_points(const struct point *source, struct point *dest, int npoints, int angle)
{
	for (int i = 0; i < npoints; i++) {
		if (source[i].x == -128) {
			dest[i] = source[i];
			continue;
		}
		rotate_point(&source[i], &dest[i], angle);
	}
}

static void draw_tank(struct tank *t)
{
	int x, y;
	struct point rotated_tank[ARRAYSIZE(tank_points)];

	rotate_points(tank_points, rotated_tank, ARRAYSIZE(tank_points), t->angle);
	x = t->x / 256;
	y = t->y / 256;
	FbDrawObject(rotated_tank, ARRAYSIZE(tank_points), t->color, x, y, 1024);
}

static void adjust_angle(int *angle, int amount)
{
	*angle += amount;
	if (*angle < 0)
		*angle += 128;
	if (*angle > 127)
		*angle -= 128;
}

static void check_buttons(void)
{
	int down_latches = button_down_latches();
	int r0 = button_get_rotation(0);
	int r1 = button_get_rotation(1);

	if (r0)
		adjust_angle(&tank[0].angle, r0);
	if (r1)
		adjust_angle(&tank[1].angle, r1);

	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		/* Pressing the button exits the program. You probably want to change this. */
		tank_vs_tank_state = TANK_VS_TANK_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	}
}

static void move_objects(void)
{
}

static void draw_screen(void)
{
	draw_tank(&tank[0]);
	draw_tank(&tank[1]);
	FbSwapBuffers();
}

static void tank_vs_tank_run(void)
{
	check_buttons();
	move_objects();
	draw_screen();
}

static void tank_vs_tank_exit(void)
{
	tank_vs_tank_state = TANK_VS_TANK_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void tank_vs_tank_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (tank_vs_tank_state) {
	case TANK_VS_TANK_INIT:
		tank_vs_tank_init();
		break;
	case TANK_VS_TANK_RUN:
		tank_vs_tank_run();
		break;
	case TANK_VS_TANK_EXIT:
		tank_vs_tank_exit();
		break;
	default:
		break;
	}
}

