#include <stdlib.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "utils.h"
#include "trig.h"
#include "xorshift.h"

#define INITIAL_ANGLE 64+32
#define MIN_ANGLE 64
#define MAX_ANGLE 127
#define MAX_BULLETS 20
#define BULLET_VEL 200
#define FIRING_COOLDOWN 10
#define MAX_MISSILES 100

static const int max_missile_cooldown = 500;
static const int min_missile_cooldown = 200;
static int missile_cooldown = 300;
static const int missile_vel = 100; 

static int screen_changed = 0;

static struct aagunner {
	int angle;
	int currently_firing;
	int firing_cooldown;
} aagunner = {
	.angle = INITIAL_ANGLE,
	.currently_firing = 0,
};

static struct missile {
	int x, y, vx, vy;
} missile[MAX_MISSILES];
static int nmissiles;

static struct bullet {
	int x, y, vx, vy;
} bullet[MAX_BULLETS];
static int nbullets = 0;

static void add_bullet(int x, int y, int vx, int vy)
{
	if (nbullets >= MAX_BULLETS)
		return;
	bullet[nbullets].x = x;
	bullet[nbullets].y = y;
	bullet[nbullets].vx = vx;
	bullet[nbullets].vy = vy;
	nbullets++;
}

static int move_bullet(int i)
{
	int x, y;
	bullet[i].x += bullet[i].vx;
	bullet[i].y += bullet[i].vy;

	x = bullet[i].x / 256;
	y = bullet[i].y / 256;


	/* check for collisions with missiles */
	for (int j = 0; j < nmissiles;) {
		int dx = abs(x - missile[j].x / 256);
		int dy = abs(y - missile[j].y / 256);
		if (dx >= 2) {
			j++;
			continue;
		}
		if (dy >= 2) {
			j++;
			continue;
		}
		/* We hit a missile, delete it. */
		if (j < nmissiles - 1)
			missile[j] = missile[nmissiles - 1];
		else
			j++;
		nmissiles--;

		/* Also, delete this bullet */
		return 1;
	}

	/* return 1 if offscreen (dead), 0 if still alive/onscreen */
	return (x < 0 || x >= LCD_XSIZE || y < 0 || y >= 151);
}

static void move_bullets(void)
{
	if (nbullets > 0)
		screen_changed = 1;
	for (int i = 0; i < nbullets;) {
		if (move_bullet(i)) { /* bullet is dead? */
			/* delete the bullet, by swapping with the last bullet and decrementing nbullets */
			if (i < nbullets - 1)
				bullet[i] = bullet[nbullets - 1];
			nbullets--;
		} else {
			i++;
		}
	}
}

static void draw_bullet(int i)
{
	int x = bullet[i].x / 256;
	int y = bullet[i].y / 256;

	FbPoint(x, y);
}

static void draw_bullets(void)
{
	FbColor(WHITE);
	for (int i = 0; i < nbullets; i++)
		draw_bullet(i);
}

static void add_missile(int x, int y, int vx, int vy)
{
	if (nmissiles >= MAX_BULLETS)
		return;
	missile[nmissiles].x = x;
	missile[nmissiles].y = y;
	missile[nmissiles].vx = vx;
	missile[nmissiles].vy = vy;
	nmissiles++;
}

static int move_missile(int i)
{
	int x, y;
	missile[i].x += missile[i].vx;
	missile[i].y += missile[i].vy;

	x = missile[i].x / 256;
	y = missile[i].y / 256;

	/* return 1 if offscreen (dead), 0 if still alive/onscreen */
	return (x < 0 || x >= LCD_XSIZE || y < 0 || y >= 151);
}

static void move_missiles(void)
{
	if (nmissiles > 0)
		screen_changed = 1;
	for (int i = 0; i < nmissiles;) {
		if (move_missile(i)) { /* missile is dead? */
			/* delete the missile, by swapping with the last missile and decrementing nmissiles */
			if (i < nmissiles - 1)
				missile[i] = missile[nmissiles - 1];
			nmissiles--;
		} else {
			i++;
		}
	}
}

static void launch_missiles(void)
{
	static unsigned int state = 0xa5a5a5a5; 

	if (missile_cooldown > 0) {
		missile_cooldown--;
		return;
	}
	missile_cooldown = min_missile_cooldown + xorshift(&state) % (max_missile_cooldown - min_missile_cooldown);
	int x, y, vx, vy, angle;

	x = xorshift(&state) % LCD_XSIZE;
	y = 0;
	if (x < 64)
		angle = 10 + xorshift(&state) % 22;
	else
		angle = 32 + xorshift(&state) % 22;
	x = x * 256;
	y = y * 256;
	vx = (cosine(angle) * missile_vel) / 256;
	vy = (sine(angle) * missile_vel) / 256;
	add_missile(x, y, vx, vy);
}

static void draw_missile(int i)
{
	int x = missile[i].x / 256;
	int y = missile[i].y / 256;

	if (x < LCD_XSIZE - 2) {
		FbPoint(x + 1, y);
		FbPoint(x + 1, y + 1);
	}
	FbPoint(x, y);
	FbPoint(x, y + 1);
}

static void draw_missiles(void)
{
	FbColor(WHITE);
	for (int i = 0; i < nmissiles; i++)
		draw_missile(i);
}

const struct point skyline[] = {
	{ -127, 119 },
	{ -62, 123 },
	{ -62, 117 },
	{ -58, 117 },
	{ -57, 105 },
	{ -55, 116 },
	{ -52, 117 },
	{ -49, 115 },
	{ -43, 115 },
	{ -43, 107 },
	{ -39, 101 },
	{ -37, 107 },
	{ -29, 106 },
	{ -30, 98 },
	{ -22, 97 },
	{ -21, 104 },
	{ -14, 103 },
	{ -13, 108 },
	{ -6, 107 },
	{ -4, 118 },
	{ -4, 95 },
	{ 3, 94 },
	{ 4, 119 },
	{ 4, 98 },
	{ 15, 98 },
	{ 14, 105 },
	{ 20, 105 },
	{ 20, 100 },
	{ 27, 101 },
	{ 27, 119 },
	{ 28, 109 },
	{ 29, 118 },
	{ 45, 120 },
	{ 44, 106 },
	{ 49, 110 },
	{ 51, 113 },
	{ 54, 112 },
	{ 54, 109 },
	{ 57, 103 },
	{ 58, 108 },
	{ 58, 122 },
	{ 126, 123 },
	{ -128, -128 },
	{ 33, 119 },
	{ 33, 122 },
	{ -128, -128 },
	{ 40, 120 },
	{ 40, 122 },
	{ -128, -128 },
};


/* Program states.  Initial state is AAGUNNER_INIT */
enum aagunner_state_t {
	AAGUNNER_INIT,
	AAGUNNER_RUN,
	AAGUNNER_EXIT,
};

static enum aagunner_state_t aagunner_state = AAGUNNER_INIT;

static void aagunner_init(void)
{
	FbInit();
	FbClear();
	aagunner_state = AAGUNNER_RUN;
	screen_changed = 1;
	aagunner.angle = INITIAL_ANGLE;
	aagunner.currently_firing = 0;
}

static void check_buttons(void)
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		aagunner.angle = aagunner.angle - 1;
		if (aagunner.angle < MIN_ANGLE)
			aagunner.angle = MIN_ANGLE;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		aagunner.angle = aagunner.angle + 1;
		if (aagunner.angle > MAX_ANGLE)
			aagunner.angle = MAX_ANGLE;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		aagunner.currently_firing = !aagunner.currently_firing;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		aagunner_state = AAGUNNER_EXIT;
	}

	if (aagunner.firing_cooldown > 0)
		aagunner.firing_cooldown--;

	if (aagunner.firing_cooldown == 0 && aagunner.currently_firing) {
		int vx, vy;

		vx = (cosine(aagunner.angle) * BULLET_VEL) / 256;
		vy = (sine(aagunner.angle) * BULLET_VEL) / 256;
		aagunner.firing_cooldown = FIRING_COOLDOWN;
		add_bullet(256 * 64, 256 * 150, vx, vy);
	}
}

static void draw_aiming_indicator(void)
{
	int x, y;

	const int x0 = 64;
	const int y0 = 150;
	FbColor(GREEN);

	for (int i = 0; i < 8; i++) {
		x = (cosine(aagunner.angle) * (i * 20)) / 256;
		y = (sine(aagunner.angle) * (i * 20)) / 256;
		x += x0;
		y += y0;
		if (x >= 0 && x < LCD_XSIZE && y >= 0 && y < LCD_YSIZE)
			FbPoint(x, y);
		else
			break;
	}
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbDrawObject(skyline, ARRAY_SIZE(skyline), GREEN, 64, 90, 512);
	draw_aiming_indicator();
	draw_bullets();
	draw_missiles();
	FbSwapBuffers();
	screen_changed = 0;
}

static void aagunner_run(void)
{
	check_buttons();
	move_bullets();
	launch_missiles();
	move_missiles();
	draw_screen();
}

static void aagunner_exit(void)
{
	aagunner_state = AAGUNNER_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void aagunner_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (aagunner_state) {
	case AAGUNNER_INIT:
		aagunner_init();
		break;
	case AAGUNNER_RUN:
		aagunner_run();
		break;
	case AAGUNNER_EXIT:
		aagunner_exit();
		break;
	default:
		break;
	}
}

