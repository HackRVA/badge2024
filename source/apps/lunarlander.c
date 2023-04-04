/*********************************************

This is a lunar lander game for the HackRVA 2020 badge.

Author: Stephen M. Cameron <stephenmcameron@gmail.com>

**********************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"

#include "xorshift.h"

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

#define TERRAIN_SEGMENT_WIDTH 10
#define DIMFACT 64

static int lander_time = 0;
char lunar_lander_msg[20] = { 0 };
int lunar_lander_msg_timer = 30;
int mission_success = 0;
int astronauts_rescued = 0;

/* Program states.  Initial state is LUNARLANDER_INIT */
enum lunarlander_state_t {
	LUNARLANDER_INIT,
	LUNARLANDER_RUN,
	LUNARLANDER_EXIT
};

static const struct point lander_points[] = {
	{ 0, -10 }, /* body */
	{ 5, -8 },
	{ 5, 0 },
	{ 0, 3 },
	{ -5, 0 },
	{ -5, -8 },
	{ 0, -10 },
	{ -128, -128 },
	{ 0, 3 }, /* thruster */
	{ -3, 6 },
	{ 3, 6 },
	{ 0, 3 },
	{ -128, -128 },
	{ -5, 0 }, /* left leg */
	{ -7, 6 },
	{ -7, 8 },
	{ -128, -128 },
	{ -9, 8 }, /* left foot */
	{ -5, 8 },
	{ -128, -128 },
	{ 5, 0 }, /* right leg */
	{ 7, 6 },
	{ 7, 8 },
	{ -128, -128 },
	{ 9, 8 }, /* right foot */
	{ 5, 8 },
	{ -128, -128 },
	{ 0, -7 }, /* window */
	{ 3, -4 },
	{ 0, -1 },
	{ -3, -4 },
	{ 0, -7 },
};

static const struct point astronaut_points[] = {
	{ -3, 8 },
	{ -2, 2 },
	{ 0, 0 },
	{ 2, 2 },
	{ 2, 8 },
	{ -128, -128 },
	{ -1, 1 },
	{ -1, -7 },
	{ 3, -6 },
	{ 2, 2 },
	{ -128, -128 },
	{ 3, -5 },
	{ 4, -2 },
	{ 3, 2 },
	{ -128, -128 },
	{ -1, -6 },
	{ -4, -5 },
	{ -8, -9 },
	{ -8, -12 },
	{ -128, -128 },
	{ 0, -8 },
	{ -1, -10 },
	{ 0, -10 },
	{ 2, -8 },
	{ 3, -6 },
};

static const struct point lunar_base_points[] = {
	{ -27, 6 },
	{ -28, 0 },
	{ 24, -1 },
	{ 31, 4 },
	{ -27, 4 },
	{ -128, -128 },
	{ -26, -2 },
	{ -24, -8 },
	{ -22, -15 },
	{ -14, -19 },
	{ -5, -22 },
	{ 5, -22 },
	{ 14, -17 },
	{ 21, -9 },
	{ 23, -2 },
	{ -128, -128 },
	{ -19, -11 },
	{ -19, -7 },
	{ -15, -7 },
	{ -15, -11 },
	{ -19, -10 },
	{ -128, -128 },
	{ -8, -11 },
	{ -9, -7 },
	{ -6, -7 },
	{ -6, -11 },
	{ -7, -11 },
	{ -128, -128 },
	{ 4, -10 },
	{ 5, -6 },
	{ 8, -6 },
	{ 7, -10 },
	{ 3, -10 },
	{ -128, -128 },
	{ 13, -1 },
	{ 13, -13 },
	{ 18, -12 },
	{ 18, -1 },
	{ -128, -128 },
	{ -26, -1 },
	{ -25, -27 },
	{ -31, -24 },
	{ -26, -23 },
};

#define NUM_TERRAIN_POINTS 256
static int terrain_y[NUM_TERRAIN_POINTS] = { 0 };

static struct lander_data {
	/* All values are 8x what is displayed. */
	int x, y;
	int vx, vy;
	int fuel;
#define FULL_FUEL (1 << 16)
#define HORIZONTAL_FUEL (256 * 3)
#define VERTICAL_FUEL (768 * 3)
	int alive;
} lander;

#define NUM_LANDING_ZONES 5
struct fuel_tank {
	int x, y;
} fueltank[NUM_LANDING_ZONES];

#define NUM_ASTRONAUTS 5
struct astronaut {
	int x, y;
	unsigned char picked_up;
} astronaut[NUM_ASTRONAUTS];

struct lunar_base {
	int x, y;
} lunar_base;

#define MAXSPARKS 50
static struct spark_data {
	int x, y, vx, vy, alive;
} spark[MAXSPARKS] = { 0 };

static unsigned int xorshift_state = 0xa5a5a5a5;

static void add_spark(int x, int y, int vx, int vy)
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive) {
			spark[i].x = x;
			spark[i].y = y;
			spark[i].vx = vx + ((xorshift(&xorshift_state) >> 16) & 0x0ff) - 128;
			spark[i].vy = vy + ((xorshift(&xorshift_state) >> 16) & 0x0ff) - 128;
			spark[i].alive = 2 + ((xorshift(&xorshift_state) >> 16) & 0x7);
			return;
		}
	}
}

static void draw_lunar_lander_msg(int color)
{
	FbMove(5, 15);
	FbColor(color);
	FbWriteLine(lunar_lander_msg);
}

static void set_message(char *msg, int time)
{
	draw_lunar_lander_msg(BLACK);
	strncpy(lunar_lander_msg, msg, sizeof(lunar_lander_msg));
	lunar_lander_msg[19] = '\0';
	lunar_lander_msg_timer = time;
	draw_lunar_lander_msg(YELLOW);
}

static void add_sparks(int x, int y, int vx, int vy, int n)
{
	int i;

	for (i = 0; i < n; i++)
		add_spark(x, y, vx, vy);
}

static void explosion(struct lander_data *lander)
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		spark[i].x = lander->x;
		spark[i].y = lander->y;
		spark[i].vx = ((xorshift(&xorshift_state) >> 16) & 0xff) - 128;
		spark[i].vy = ((xorshift(&xorshift_state) >> 16) & 0xff) - 128;
		spark[i].alive = 100;
	}
	set_message("MISSION FAILED", 60);
}

static void move_sparks(void)
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive)
			continue;
		spark[i].x += spark[i].vx;
		spark[i].y += spark[i].vy;
		if (spark[i].alive > 0)
			spark[i].alive--;
	}
}

static void draw_fuel_gauge_ticks(void)
{
	int i;
	FbColor(WHITE);
	FbVerticalLine(127, 5, 127, 105);
	for (i = 0; i <= 10; i++)
		FbHorizontalLine(120, 5 + i * 10, 126, 5 + i * 10);
}

static void draw_fuel_gauge_marker(struct lander_data *lander, int color)
{
	int y1, y2, y3;

	y2 = 106 - ((100 * lander->fuel) >> 16);
	y1 = y2 - 5;
	y3 = y2 + 5;
	FbColor(color);
	FbVerticalLine(115, y1, 115, y3);
	FbLine(115, y1, 122, y2);
	FbLine(115, y3, 122, y2);
}

static void draw_fuel_gauge(struct lander_data *lander, int color)
{
	if (color != BLACK)
		draw_fuel_gauge_ticks();
	draw_fuel_gauge_marker(lander, color);
	if (lander->fuel == 0) {
		FbColor(YELLOW);
		FbMove(2, 110);
		FbWriteLine("OUT OF FUEL");	
	}
}

static void draw_sparks(struct lander_data *lander, int color)
{
	int i, x1, y1, x2, y2;
	const int sx = LCD_XSIZE / 2;
	const int sy = LCD_YSIZE / 3;

	FbColor(color);
	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive)
			continue;
		x1 = ((spark[i].x - lander->x - spark[i].vx) >> 8) + sx;
		y1 = ((spark[i].y - lander->y - spark[i].vy) >> 8) + sy;
		x2 = ((spark[i].x - lander->x) >> 8) + sx;
		y2 = ((spark[i].y - lander->y) >> 8) + sy;
		if (x1 >= 0 && x1 <= 127 && y1 >= 0 && y1 <= 127 &&
			x2 >= 0 && x2 <= 127 && y2 >= 0 && y2 <= 127)
			FbLine(x1, y1, x2, y2);
	}
}

static enum lunarlander_state_t lunarlander_state = LUNARLANDER_INIT;

static void init_terrain(int start, int stop)
{
	int mid, midy, n;

	mid = start + ((stop - start) / 2);

	if (mid == start || mid == stop)
		return;
	midy = (terrain_y[start] + terrain_y[stop]) / 2;

	/* Make sure we don't make accidental landing zones */
	if (midy == terrain_y[start])
		midy = midy + 1;
	if (midy == terrain_y[stop])
		midy = midy + 1;

	/* Displace midy randomly here in proportion to (stop - start). */
	n = (xorshift(&xorshift_state) % 10000) - 5000;
	n = 3 * (n * (stop - start)) / 10000;
	midy += n;

	terrain_y[mid] = midy;
	init_terrain(start, mid);
	init_terrain(mid, stop);
	lander_time = 0;
}

static void add_landing_zones(int t[], int start, int stop, int count)
{
	int d = (stop - start) / (count + 1);
	int i, x;

	x = d;

	/* Flatten out some terrain for a landing zone. */
	for (i = 0; i < count; i++) {
		t[x] = t[x + 1];
		t[x + 2] = t[x];
		fueltank[i].x = x;
		fueltank[i].y = 0;
		x += d;
	}
}

static void place_astronauts(void)
{
	int i, j;

	for (i = 0; i < NUM_ASTRONAUTS; i++) {
		j = (xorshift(&xorshift_state) % ((80 * NUM_TERRAIN_POINTS) / 100)) +
				(10 * NUM_TERRAIN_POINTS) / 100;
		astronaut[i].y = terrain_y[j] - 5;
		astronaut[i].x = j * TERRAIN_SEGMENT_WIDTH;
		astronaut[i].picked_up = 0;
	}
}

static void place_lunar_base(void)
{
	int j;
	j = (xorshift(&xorshift_state) % 25) + (85 * NUM_TERRAIN_POINTS) / 100;
	lunar_base.y = terrain_y[j] - 5;
	lunar_base.x = j * TERRAIN_SEGMENT_WIDTH;
}

static void lunarlander_init(void)
{
	FbInit();
	FbClear();
	terrain_y[0] = -100;
	terrain_y[NUM_TERRAIN_POINTS - 1] = -100;
	init_terrain(0, NUM_TERRAIN_POINTS - 1);
	add_landing_zones(terrain_y, 0, NUM_TERRAIN_POINTS - 1, NUM_LANDING_ZONES);
	place_astronauts();
	place_lunar_base();
	lunarlander_state = LUNARLANDER_RUN;
	lander.x = 100 << 8;
	lander.y = (terrain_y[9] - 60) << 8;
	lander.vx = 0;
	lander.vy = 0;
	lander.fuel = FULL_FUEL;
	lander.alive = 1;
	mission_success = 0;
	set_message("MOVE RIGHT", 30);
}

static void reduce_fuel(struct lander_data *lander, int amount)
{
	draw_fuel_gauge(lander, BLACK);
	lander->fuel -= amount;
	if (lander->fuel < 0)
		lander->fuel = 0;
}

static void check_buttons()
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		/* Pressing the button exits the program. You probably want to change this. */
		lunarlander_state = LUNARLANDER_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		if (lander.fuel > 0) {
			lander.vx = lander.vx - (1 << 7);
			add_sparks(lander.x + (5 << 8), lander.y, lander.vx + (5 << 8), lander.vy, 10);
			reduce_fuel(&lander, HORIZONTAL_FUEL);
		}
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		if (lander.fuel > 0) {
			lander.vx = lander.vx + (1 << 7);
			add_sparks(lander.x - (5 << 8), lander.y, lander.vx - (5 << 8), lander.vy, 10);
			reduce_fuel(&lander, HORIZONTAL_FUEL);
		}
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		if (lander.fuel > 0) {
			lander.vy = lander.vy - (1 << 7);
			add_sparks(lander.x, lander.y + (5 << 8), lander.vx, lander.vy + (5 << 8), 10);
			reduce_fuel(&lander, VERTICAL_FUEL);
		}
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	}
}

static void draw_lander(void)
{
	const int x = LCD_XSIZE / 2;
	const int y = LCD_YSIZE / 3;
	FbDrawObject(lander_points, ARRAYSIZE(lander_points), BLACK, x, y, 1024);
	if (lander.alive > 0)
		FbDrawObject(lander_points, ARRAYSIZE(lander_points), WHITE, x, y, 1024);
}

static void draw_terrain_segment(struct lander_data *lander, int i, int color)
{
	int x1, y1, x2, y2, sx1, sy1, sx2, sy2, j;
	int left = (lander->x >> 8) - DIMFACT * LCD_XSIZE / 2;
	int right = (lander->x >> 8) + DIMFACT * LCD_XSIZE / 2;
	int top = (lander->y >> 8) - DIMFACT * LCD_YSIZE / 3;
	int bottom = (lander->y >> 8) + DIMFACT * 2 * LCD_YSIZE / 3;

	if (i < 0)
		return;
	if (i > NUM_TERRAIN_POINTS - 2)
		return;
	x1 = i * TERRAIN_SEGMENT_WIDTH;
	if (x1 <= left || x1 >= right)
		return;
	sx1 = x1 - (lander->x >> 8) + LCD_XSIZE / 2;
	if (sx1 < 0 || sx1 >= LCD_XSIZE)
		return;
	x2 = (i + 1) * TERRAIN_SEGMENT_WIDTH;
	if (x2 <= left || x2 >= right)
		return;
	sx2 = x2 - (lander->x >> 8) + LCD_XSIZE / 2;
	if (sx2 < 0 || sx2 >= LCD_XSIZE)
		return;
	y1 = terrain_y[i];
	if (y1 < top || y1 >= bottom)
		return;
	sy1 = y1 - (lander->y >> 8) + LCD_YSIZE / 3;
	if (sy1 < 0 || sy1 >= LCD_YSIZE)
		return;
	y2 = terrain_y[i + 1];
	if (y2 < top || y2 >= bottom)
		return;
	sy2 = y2 - (lander->y >> 8) + LCD_YSIZE / 3;
	if (sy2 < 0 || sy2 >= LCD_YSIZE)
		return;
	if (x1 <= (lander->x >> 8) && x2 >= (lander->x >> 8) && color != BLACK) {
		if ((lander->y >> 8) >= y2 - 8) {
			if (lander->alive > 0 && y2 != y1) {
				/* Explode lander if not on level ground */
				FbColor(color);
				explosion(lander);
				lander->alive = -100;
			} else {
				/* Explode lander if it hits too hard. */
				if ((lander->vy > 256 || lander->vx > 256 || lander->vx < -256) && lander->alive > 0) {
					explosion(lander);
					lander->alive = -100;
				} else {
					if (lander->vy > 0)
						lander->vy = 0; /* Allow lander to land */
					for (j = 0; j < NUM_LANDING_ZONES; j++) {
						if (abs((lander->x >> 8) - fueltank[j].x * TERRAIN_SEGMENT_WIDTH) < 2 * TERRAIN_SEGMENT_WIDTH) {
							draw_fuel_gauge(lander, BLACK);
							lander->fuel = FULL_FUEL; /* refuel lander */
						}
					}
				}
			}
		} else {
			FbColor(color);
		}
	} else {
#if 0
		if (y2 == y1 && color != BLACK) /* Make landing zones green */
			FbColor(GREEN);
		else
#endif
			FbColor(color);
	}
	FbLine(sx1, sy1, sx2, sy2);
	/* Draw a flag by landing zones. */
	for (j = 0; j < NUM_LANDING_ZONES; j++) {
		if (i == fueltank[j].x) {
			int y2 = sy1 - 20;
			if (y2 < 0)
				y2 = 0;
			if (color != BLACK)
				FbColor(GREEN);
			FbLine(sx2, sy1, sx2, y2);
			if (y2 < 120 && sx2 < 120) {
				FbLine(sx2, y2, sx2 + 7, y2 + 3);
				FbLine(sx2 + 7, y2 + 3, sx2, y2 + 6);
			}
			set_message("REFUEL AT FLAG", 30);
			break;
		}
	}
}

static void draw_terrain(struct lander_data *lander, int color)
{
	int start, stop, i;

	start = ((lander->x >> 8) - TERRAIN_SEGMENT_WIDTH * LCD_XSIZE / 2) / TERRAIN_SEGMENT_WIDTH;
	if (start < 0)
		start = 0;
	if (start > NUM_TERRAIN_POINTS - 1)
		return;
	stop = ((lander->x >> 8) + TERRAIN_SEGMENT_WIDTH * LCD_XSIZE / 2) / TERRAIN_SEGMENT_WIDTH;
	if (stop > NUM_TERRAIN_POINTS - 1)
		stop = NUM_TERRAIN_POINTS - 1;
	FbColor(color);
	for (i = start; i < stop; i++)
		draw_terrain_segment(lander, i, color);
}

static void draw_astronaut(struct lander_data *lander, int i, int color)
{
	int x, y;

	x = astronaut[i].x - (lander->x >> 8) + LCD_XSIZE / 2;
	y = astronaut[i].y - (lander->y >> 8) + LCD_YSIZE / 3;
	if (x < 10 || x > LCD_XSIZE - 10)
		return;
	if (y < 10 || y > LCD_YSIZE - 10)
		return;
	FbDrawObject(astronaut_points, ARRAYSIZE(astronaut_points), color, x, y, 512);
	set_message("RESCUE BUDDIES!", 30);
	if (abs(astronaut[i].x - (lander->x >> 8)) < 9 && abs(astronaut[i].y - (lander->y >>8)) < 9) {
		set_message("WOOHOO!", 60);
		astronaut[i].picked_up = 1;
	}
}

static void draw_lunar_base(struct lander_data *lander, int color)
{
	int i, x, y;

	x = lunar_base.x - (lander->x >> 8) + LCD_XSIZE / 2;
	y = lunar_base.y - (lander->y >> 8) + LCD_YSIZE / 3;
	if (x < 20 || x > LCD_XSIZE - 20)
		return;
	if (y < 20 || y > LCD_YSIZE - 20)
		return;
	FbDrawObject(lunar_base_points, ARRAYSIZE(lunar_base_points), color, x, y, 512);
	set_message("LAND ON BASE!", 30);
	if (abs(lunar_base.x - (lander->x >> 8)) < 20 && abs(lunar_base.y - (lander->y >>8)) < 20) {
		if (astronauts_rescued > 0)
			set_message("MISSION SUCCESS", 120);
		else
			set_message("MISSION FAILED", 120);
		if (mission_success == 0)
			mission_success = 60;
		astronauts_rescued = 0;
		for (i = 0; i < NUM_ASTRONAUTS; i++)
			if (astronaut[i].picked_up)
				astronauts_rescued++;
	}
}

static void draw_astronauts(struct lander_data *lander, int color)
{
	int i;

	for (i = 0; i < NUM_ASTRONAUTS; i++) {
		if (astronaut[i].picked_up)
			continue;
		draw_astronaut(lander, i, color);
		if (astronaut[i].picked_up)
			draw_astronaut(lander, i, BLACK);
	}
}

static void move_lander(void)
{
	if (lander.alive < 0) {
		lander.alive++;
		if (lander.alive == 0) {
			lunarlander_state = LUNARLANDER_INIT;
		}
		return;
	}
	lander_time++;
	if (lander_time > 10000)
		lander_time = 0;
	if ((lander_time & 0x08) == 0) {
		lander.vy += 10;
	}
	lander.y += lander.vy;
	lander.x += lander.vx;
	if (lander.x < TERRAIN_SEGMENT_WIDTH << 8)
		lander.x += ((int) (NUM_TERRAIN_POINTS - 2) * 10 - TERRAIN_SEGMENT_WIDTH) << 8;
	if (lander.x >= (((int) (NUM_TERRAIN_POINTS - 1) * 10) << 8))
		lander.x = (TERRAIN_SEGMENT_WIDTH << 8);
}

static void update_message(void)
{
	if (lunar_lander_msg_timer > 0)
		lunar_lander_msg_timer--;
	if (lunar_lander_msg_timer == 0)
		set_message("", 1);
}

static void draw_stats(void)
{
	char buffer[10];

	FbColor(YELLOW);
	FbMove(5, 75);
	FbWriteLine("BUDDIES");
	FbMove(5, 90);
	sprintf( buffer, "%d", astronauts_rescued);
	FbWriteLine("RESCUED: ");
	FbMove(80, 90);
	FbWriteLine(buffer);
	FbMove(5, 105);
	FbWriteLine("OUT OF 5");
	FbMove(5, 120);
	if (astronauts_rescued < 1)
		FbWriteLine("DISASTER!");
	else if (astronauts_rescued < 5)
		FbWriteLine("TRAGEDY!");
	else
		FbWriteLine("GOOD JOB!");
}

static void draw_screen()
{
	FbColor(WHITE);
	draw_terrain(&lander, BLACK); /* Erase previously drawn terrain */
	draw_sparks(&lander, BLACK);
	draw_astronauts(&lander, BLACK);
	draw_lunar_base(&lander, BLACK);
	if (mission_success == 0) {
		move_lander();
	} else {
		mission_success--;
		if (mission_success == 0)
			lunarlander_state = LUNARLANDER_INIT; /* start over */
		draw_stats();
	}
	move_sparks();
	update_message();
	draw_terrain(&lander, WHITE); /* Draw terrain */
	draw_lander();
	draw_astronauts(&lander, GREEN);
	draw_lunar_base(&lander, CYAN);
	draw_fuel_gauge(&lander, RED);
	draw_sparks(&lander, YELLOW);
	FbSwapBuffers();
}

static void lunarlander_run()
{
	if (mission_success == 0)
		check_buttons();
	draw_screen();
}

static void lunarlander_exit()
{
	lunarlander_state = LUNARLANDER_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

int lunarlander_cb(void)
{
	switch (lunarlander_state) {
	case LUNARLANDER_INIT:
		lunarlander_init();
		break;
	case LUNARLANDER_RUN:
		lunarlander_run();
		break;
	case LUNARLANDER_EXIT:
		lunarlander_exit();
		break;
	default:
		break;
	}
	return 0;
}
