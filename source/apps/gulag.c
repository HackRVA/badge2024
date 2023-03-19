#include <stdio.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "trig.h"

/* Program states.  Initial state is GULAG_INIT */
enum gulag_state_t {
	GULAG_INIT,
	GULAG_INTRO,
	GULAG_FLAG,
	GULAG_RUN,
	GULAG_EXIT,
};

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#ifdef TARGET_PICO
/* printf?  What printf? */
#define printf(...)
#endif

static const short player_speed = 512;

static struct player {
	short room;
	short x, y; /* 8.8 signed fixed point */
	short oldx, oldy;
	unsigned char angle, oldangle; /* 0 - 127, 0 is to the left, 32 is down, 64 is right, 96 is up. */
} player;

static enum gulag_state_t gulag_state = GULAG_INIT;
static int screen_changed = 0;
static int intro_offset = 0;
static int flag_offset = 0;

static void init_player(struct player *p)
{
	p->room = 0;
	p->x = 20 << 8;
	p->y = 20 << 8;
	p->oldx = p->x;
	p->oldy = p->y;
	p->angle = 0;
	p->oldangle = 0;
}

static void gulag_init(void)
{
	init_player(&player);
	FbInit();
	FbClear();
	gulag_state = GULAG_INTRO;
	screen_changed = 1;
	intro_offset = 0;
	flag_offset = 0;
}

static void gulag_intro(void)
{
	int first_line;
	int y;

	FbColor(YELLOW);
	static const char *intro_text[] = {
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"YOU ARE A",
		"UKRANIAN",
		"SOLDIER",
		"CAPTURED BY THE",
		"RUSSIAN ARMY",
		"AND IMPRISONED.",
		"",
		"BUT YOU HAVE",
		"A DARING PLAN",
		"TO ESCAPE,",
		"PLANT A BOMB",
		"IN THE ARMORY",
		"AND STEAL THE",
		"BATTLE PLANS",
		"OF YOUR RUSSIAN",
		"CAPTORS.",
		"",
		"YOU HAVE JUST",
		"PICKED THE LOCK",
		"ON YOUR CELL",
		"DOOR AND YOU",
		"NOW BEGIN TO",
		"PUT YOUR BOLD",
		"PLAN INTO",
		"ACTION!",
		"",
		"SLAVA UKRAINI!",
		"",
		"",
		"",
		"",
		"",
		0,
	};

	if (intro_offset == 425) {
		gulag_state = GULAG_FLAG;
		return;
	}

	first_line = intro_offset / 10;

	y = -(intro_offset) % 10;
	y = y + 12;
	for (int i = first_line; i < first_line + 11; i++) {
		if (i >= (int) ARRAYSIZE(intro_text)) {
			i++;
			y = y + 10;
			continue;
		}
		if (!intro_text[i])
			continue;
		FbMove(5, y);
		FbWriteLine(intro_text[i]);
		y = y + 10;
	}
	FbColor(BLACK);
	FbMove(0, 0);
	FbFilledRectangle(127, 10);
	FbColor(BLACK);
	FbMove(0, 110);
	FbFilledRectangle(127, 10);
	intro_offset++;
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		gulag_state = GULAG_RUN;
}

static void gulag_flag(void)
{
	if (flag_offset == 65) {
		flag_offset = 0;
		gulag_state = GULAG_RUN;
		return;
	}
	FbMove(0, 0);
	FbColor(BLUE);
	FbFilledRectangle(127, flag_offset);
	FbMove(0, 127 - flag_offset);
	FbColor(YELLOW);
	FbFilledRectangle(127, flag_offset);
	FbSwapBuffers();
	flag_offset++;

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		gulag_state = GULAG_RUN;
}

static void check_buttons()
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		/* Pressing the button exits the program. You probably want to change this. */
		// gulag_state = GULAG_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		short new_angle = player.angle + 3;
		if (new_angle > 127)
			new_angle -= 127;
		player.oldangle = player.angle;
		player.angle = (unsigned char) new_angle;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		short new_angle = player.angle - 3;
		if (new_angle < 0)
			new_angle += 127;
		player.oldangle = player.angle;
		player.angle = (unsigned char) new_angle;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_ENCODER_B, down_latches)) {
		int newx, newy;
		newx = ((-cosine(player.angle) * player_speed) >> 8) + player.x;
		newy = ((sine(player.angle) * player_speed) >> 8) + player.y;

		if (newx < 5 << 8)
			newx = 5 << 8;
		if (newx > ((127 - 4) << 8))
			newx = (127 - 4) << 8;
		if (newy < 9 << 8)
			newy = 9 << 8;
		if (newy > ((127 -16 - 8) << 8))
			newy = (127 - 16 - 8) << 8;
		player.oldx = player.x;
		player.oldy = player.y;
		player.x = (short) newx;
		player.y = (short) newy;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	}
}

static void constrain_to_screen(short *x)
{
	if (*x < 0)
		*x = 0;
	if (*x > 127)
		*x = 127;
}

static void draw_plus(short x, short y)
{
	short x1, y1, x2, y2;
	constrain_to_screen(&x);
	constrain_to_screen(&y);
	x1 = x - 2;
	y1 = y - 2;
	x2 = x + 2;
	y2 = y + 2;
	constrain_to_screen(&x1);
	constrain_to_screen(&y1);
	constrain_to_screen(&x2);
	constrain_to_screen(&y2);
	FbHorizontalLine(x1, y, x2, y);
	FbVerticalLine(x, y1, x, y2);
}

static void erase_player(struct player *p)
{
	short x, y, w, h;
	int dx, dy;

	x = p->oldx >> 8;
	y = p->oldy >> 8;
	x = x - 4;
	y = y - 8;
	w = x + 8;
	if (w >= 127)
		w = 126;
	w = w - x;
	h = y + 16;
	if (h >= 127)
		h = 126;
	h = h - y;
	FbColor(BLACK);
	FbMove(x, y);
	FbFilledRectangle(w, h);
	dx = ((-cosine(p->oldangle) * 16 * player_speed) >> 8) + p->oldx;
	dy = ((sine(p->oldangle) * 16 * player_speed) >> 8) + p->oldy;
	x = (short) (dx >> 8);
	y = (short) (dy >> 8);
	FbColor(BLACK);
	draw_plus(x, y);
}

static void draw_player(struct player *p)
{
	short x, y, x1, y1, x2, y2;
	int dx, dy;

	erase_player(p);
	x = p->x >> 8;
	y = p->y >> 8;

	x1 = x - 4;
	x2 = x + 3;
	y1 = y - 8;
	y2 = y + 7;
	constrain_to_screen(&x1);
	constrain_to_screen(&y1);
	constrain_to_screen(&x2);
	constrain_to_screen(&y2);

	FbColor(WHITE);
	FbHorizontalLine(x1, y1, x2, y1);
	FbHorizontalLine(x1, y2, x2, y2);
	FbVerticalLine(x1, y1, x1, y2);
	FbVerticalLine(x2, y1, x2, y2);
	dx = ((-cosine(p->angle) * 16 * player_speed) >> 8) + p->x;
	dy = ((sine(p->angle) * 16 * player_speed) >> 8) + p->y;
	x = (short) (dx >> 8);
	y = (short) (dy >> 8);
	FbColor(CYAN);
	draw_plus(x, y);

}

static void draw_room(void)
{
	FbHorizontalLine(0, 0, 127, 0);
	FbVerticalLine(0, 0, 0, 127 - 16);
	FbHorizontalLine(0, 127 - 16, 127, 127 - 16);
	FbVerticalLine(127, 0, 127, 127 - 16);
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	draw_room();
	draw_player(&player);
	FbSwapBuffers();
	screen_changed = 0;
}

static void gulag_run()
{
	check_buttons();
	draw_screen();
}

static void gulag_exit()
{
	gulag_state = GULAG_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void gulag_cb(void)
{
	switch (gulag_state) {
	case GULAG_INIT:
		gulag_init();
		break;
	case GULAG_INTRO:
		gulag_intro();
		break;
	case GULAG_FLAG:
		gulag_flag();
		break;
	case GULAG_RUN:
		gulag_run();
		break;
	case GULAG_EXIT:
		gulag_exit();
		break;
	default:
		break;
	}
}

