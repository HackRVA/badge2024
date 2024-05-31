#include <stdlib.h>
#include <stdio.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "utils.h"
#include "trig.h"
#include "xorshift.h"
#include "dynmenu.h"

#define INITIAL_ANGLE 64+32
#define MIN_ANGLE 64
#define MAX_ANGLE 127
#define MAX_BULLETS 40
#define BULLET_VEL 200
#define FIRING_COOLDOWN 10
#define MAX_MISSILES 100
#define MAX_SPARKS 400
#define SPARKS_PER_MISSILE 10

static int max_missile_cooldown = 500;
static int min_missile_cooldown = 200;
static int missile_cooldown = 300;
static int missile_vel = 100; 
static int missiles_killed = 0;
static int missiles_killed_this_wave = 0;
static int shots_fired = 0;
static int shots_fired_this_wave = 0;
static int missile_impacts = 0;
static int missile_impacts_this_wave = 0;
static int missiles_this_wave = 0;
static int casualties = 0;
static int total_casualties = 0;

static int screen_changed = 0;

static struct aagunner_wave {
	char *name;
	int missile_count;
	int min_missile_cooldown;
	int max_missile_cooldown;
	int missile_vel;
	int mirv_chance; /* out of 255 */
} wave[] = {
	{
		.name = "LUHANSK",
		.missile_count = 5,
		.min_missile_cooldown = 200,
		.max_missile_cooldown = 500,
		.missile_vel = 100,
		.mirv_chance = 0,
	},
	{
		.name = "HORLIVKA",
		.missile_count = 5,
		.min_missile_cooldown = 100,
		.max_missile_cooldown = 400,
		.missile_vel = 120,
		.mirv_chance = 0,
	},
	{
		.name = "KRAMATORSK",
		.missile_count = 8,
		.min_missile_cooldown = 50,
		.max_missile_cooldown = 300,
		.missile_vel = 150,
		.mirv_chance = 0,
	},
	{
		.name = "DONETSK",
		.missile_count = 8,
		.min_missile_cooldown = 50,
		.max_missile_cooldown = 250,
		.missile_vel = 150,
		.mirv_chance = 0,
	},
	{
		.name = "ODESA",
		.missile_count = 8,
		.min_missile_cooldown = 25,
		.max_missile_cooldown = 250,
		.missile_vel = 150,
		.mirv_chance = 0,
	},
	{
		.name = "MYKOLAIV",
		.missile_count = 8,
		.min_missile_cooldown = 20,
		.max_missile_cooldown = 200,
		.missile_vel = 150,
		.mirv_chance = 50,
	},
	{
		.name = "KUPYANSK",
		.missile_count = 10,
		.min_missile_cooldown = 20,
		.max_missile_cooldown = 200,
		.missile_vel = 175,
		.mirv_chance = 50,
	},
	{
		.name = "BAKHMUT",
		.missile_count = 12,
		.min_missile_cooldown = 20,
		.max_missile_cooldown = 200,
		.missile_vel = 175,
		.mirv_chance = 75,
	},
	{
		.name = "ZAPORIZHZHIA",
		.missile_count = 20,
		.min_missile_cooldown = 20,
		.max_missile_cooldown = 150,
		.missile_vel = 175,
		.mirv_chance = 128,
	},
	{
		.name = "KYIV",
		.missile_count = 50,
		.min_missile_cooldown = 10,
		.max_missile_cooldown = 100,
		.missile_vel = 190,
		.mirv_chance = 255,
	},
};

static const int nwaves = ARRAY_SIZE(wave);
static int current_wave = 0;
static const int wave_pause_time = 100;

static struct aagunner {
	int angle;
	int currently_firing;
	int firing_cooldown;
} aagunner = {
	.angle = INITIAL_ANGLE,
	.currently_firing = 0,
};

static struct spark {
	int x, y, vx, vy, life, color;
} spark[MAX_SPARKS];
static int nsparks = 0;

static struct missile {
	int x, y, vx, vy;
	int mirv_count;
} missile[MAX_MISSILES];
static int nmissiles;

static struct bullet {
	int x, y, vx, vy;
} bullet[MAX_BULLETS];
static int nbullets = 0;

static struct dynmenu main_menu;
static struct dynmenu_item menu_item[15];

static void add_spark(int x, int y, int vx, int vy, int color)
{
	static unsigned int state = 0xa5a5a5a5;

	if (nsparks >= MAX_SPARKS)
		return;
	spark[nsparks].x = x;
	spark[nsparks].y = y;
	spark[nsparks].vx = vx;
	spark[nsparks].vy = vy;
	spark[nsparks].life = xorshift(&state) % 100 + 100;
	spark[nsparks].color = color;
	nsparks++;
}

static int move_spark(int i)
{
	int x, y;
	spark[i].x += spark[i].vx;
	spark[i].y += spark[i].vy;
	spark[i].life--;
	if (spark[i].life == 0)
		return 1; /* spark is dead */

	x = spark[i].x / 256;
	y = spark[i].y / 256;

	/* return 1 if offscreen (dead), 0 if still alive/onscreen */
	return (x < 0 || x >= LCD_XSIZE || y < 0 || y >= 151);
}

static void move_sparks(void)
{
	if (nsparks > 0)
		screen_changed = 1;
	for (int i = 0; i < nsparks;) {
		if (move_spark(i)) { /* spark is dead? */
			/* delete the spark, by swapping with the last spark and decrementing nsparks */
			if (i < nsparks - 1)
				spark[i] = spark[nsparks - 1];
			nsparks--;
		} else {
			i++;
		}
	}
}

static void draw_spark(int i)
{
	int x = spark[i].x / 256;
	int y = spark[i].y / 256;

	FbColor(spark[i].color);
	FbPoint(x, y);
}

static void draw_sparks(void)
{
	for (int i = 0; i < nsparks; i++)
		draw_spark(i);
}

static void add_bullet(int x, int y, int vx, int vy)
{
	if (nbullets >= MAX_BULLETS)
		return;
	bullet[nbullets].x = x;
	bullet[nbullets].y = y;
	bullet[nbullets].vx = vx;
	bullet[nbullets].vy = vy;
	nbullets++;
	shots_fired++;
	shots_fired_this_wave++;
}

static int move_bullet(int i)
{
	static unsigned int state = 0xa5a5a5a5;
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

		/* We hit a missile, add some sparks...  */
		for (int i = 0; i < SPARKS_PER_MISSILE; i++) {
			int x = missile[j].x;
			int y = missile[j].y;
			int angle = xorshift(&state) % 127;
			int vx = missile[j].vx + (cosine(angle) * missile_vel) / 256;
			int vy = missile[j].vy + (sine(angle) * missile_vel) / 256;

			vx = (vx * (xorshift(&state) % 256)) / 256;
			vy = (vy * (xorshift(&state) % 256)) / 256;

			add_spark(x, y, vx, vy, YELLOW);
		}
		missiles_killed++;
		missiles_killed_this_wave++;

		/* ... and delete it. */
		if (j < nmissiles - 1)
			missile[j] = missile[nmissiles - 1];
		else
			j++;
		nmissiles--;
		missiles_this_wave++;

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


static void add_missile(int x, int y, int vx, int vy, int mirv_count)
{
	if (nmissiles >= MAX_MISSILES)
		return;
	missile[nmissiles].x = x;
	missile[nmissiles].y = y;
	missile[nmissiles].vx = vx;
	missile[nmissiles].vy = vy;
	missile[nmissiles].mirv_count = mirv_count;
	nmissiles++;
}

static int move_missile(int i)
{
	static unsigned int state = 0xa5a5a5a5;

	int x, y;
	missile[i].x += missile[i].vx;
	missile[i].y += missile[i].vy;

	x = missile[i].x / 256;
	y = missile[i].y / 256;

	if ((xorshift(&state) & 0x0f) == 0x01)
		add_spark(missile[i].x, missile[i].y, missile[i].vx / 2, missile[i].vy / 2, RED);

	if (y >= 40 && missile[i].mirv_count > 0) {
		struct missile *m = &missile[i];
		add_missile(m->x, m->y, m->vx + 50, m->vy, 0);
		add_missile(m->x, m->y, m->vx - 50, m->vy, 0);
		m->mirv_count = 0;
	}

	if (y >= 151) {
		for (int j = 0; j < 100; j++) {
			int angle, v, vx, vy;

			angle = 64 + xorshift(&state) % 64;
			v = (xorshift(&state) % 50) + 1;
			vx = (cosine(angle) * v) / 8;
			vy = (sine(angle) * v) / 8;
			add_spark(missile[i].x, missile[i].y, vx, vy, YELLOW);
		}
		missile_impacts++;
		missile_impacts_this_wave++;
		casualties += 10 + (64 - abs((missile[i].x / 256) - 64)) * 5;
	}

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
			missiles_this_wave++;
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

	if (missiles_this_wave > wave[current_wave].missile_count)
		return;

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

	int chance = xorshift(&state) & 0xff;
	int mirv_count = chance < wave[current_wave].mirv_chance ? 3 : 0;
	printf("mirv_count = %d, mirv_chance = %d, chance = %d\n", mirv_count, wave[current_wave].mirv_chance, chance);
	add_missile(x, y, vx, vy, mirv_count);
}

static void draw_missile(int i)
{
	int x = missile[i].x / 256;
	int y = missile[i].y / 256;

	if (missile[i].mirv_count > 0)
		FbColor(RED);
	else
		FbColor(WHITE);

	if (x < LCD_XSIZE - 2) {
		FbPoint(x + 1, y);
		FbPoint(x + 1, y + 1);
	}
	FbPoint(x, y);
	FbPoint(x, y + 1);
}

static void draw_missiles(void)
{
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
	AAGUNNER_MENU,
	AAGUNNER_NEW_GAME,
	AAGUNNER_BEGIN_WAVE,
	AAGUNNER_END_WAVE,
	AAGUNNER_RUN,
	AAGUNNER_EXIT,
};

static enum aagunner_state_t aagunner_state = AAGUNNER_INIT;

static void aagunner_init(void)
{
	FbInit();
	FbClear();
	aagunner_state = AAGUNNER_MENU;
	screen_changed = 1;
}

static void aagunner_new_game(void)
{
	aagunner.angle = INITIAL_ANGLE;
	aagunner.currently_firing = 0;
	aagunner.firing_cooldown = 0;
	current_wave = 0;
	min_missile_cooldown = wave[current_wave].min_missile_cooldown;
	max_missile_cooldown = wave[current_wave].max_missile_cooldown;
	missile_vel = wave[current_wave].missile_vel;
	missile_cooldown = wave[current_wave].max_missile_cooldown;
	nmissiles = 0;
	nbullets = 0;
	nsparks = 0;
	screen_changed = 1;
	FbClear();
	aagunner_state = AAGUNNER_BEGIN_WAVE;
	missiles_killed = 0;
	missiles_killed_this_wave = 0;
	missile_impacts = 0;
	missile_impacts_this_wave = 0;
	shots_fired = 0;
	shots_fired_this_wave = 0;
	casualties = 0;
	total_casualties = 0;
}

static void aagunner_begin_wave(void)
{
	char buf[100];
	static int screen_changed = 1;
	static int count = 0;

	if (screen_changed) {
		snprintf(buf, sizeof(buf), "WAVE %d\n%s\n\nGET READY!",
			current_wave + 1, wave[current_wave].name);
		FbClear();
		FbMove(0, 4 * 8);
		FbColor(WHITE);
		FbWriteString(buf);
		FbSwapBuffers();
		screen_changed = 0;
	}
	count++;

	if (count > wave_pause_time) {
		count = 0;
		screen_changed = 1;
		aagunner_state = AAGUNNER_RUN;
		aagunner.angle = INITIAL_ANGLE;
		aagunner.currently_firing = 0;
		aagunner.firing_cooldown = 0;
		min_missile_cooldown = wave[current_wave].min_missile_cooldown;
		max_missile_cooldown = wave[current_wave].max_missile_cooldown;
		missile_vel = wave[current_wave].missile_vel;
		missile_cooldown = wave[current_wave].min_missile_cooldown;
		missiles_this_wave = 0;
		nmissiles = 0;
		nsparks = 0;
		nbullets = 0;
		shots_fired_this_wave = 0;
		missile_impacts_this_wave = 0;
		casualties = 0;
	}
}

static void aagunner_end_wave(void)
{
	static int screen_changed = 1;
	char buf[300];

	if (screen_changed) {
		FbClear();
		snprintf(buf, sizeof(buf), "WAVE %d ENDS\n\n"
					"SHOTS:%d\n"
					"CASUALTIES:\n %d\n"
					"IMPACTS:%d\n"
					"KILLS: %d\n\n"
					"   PRESS A TO\n"
					"     CONTINUE",
			current_wave,
			shots_fired_this_wave,
			casualties,
			missile_impacts_this_wave,
			missiles_killed_this_wave);
		FbMove(0, 4 * 8);
		FbWriteString(buf);
		FbSwapBuffers();
		screen_changed = 0;
	}

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		screen_changed = 1;
		aagunner_state = AAGUNNER_BEGIN_WAVE;
	}
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
	draw_sparks();
	FbSwapBuffers();
	screen_changed = 0;
}

static void aagunner_menu(void)
{
	static int menu_ready = 0;

	if (!menu_ready) {
		dynmenu_init(&main_menu, menu_item, ARRAY_SIZE(menu_item));
		dynmenu_add_item(&main_menu, "NEW GAME", AAGUNNER_NEW_GAME, 0);
		dynmenu_add_item(&main_menu, "QUIT", AAGUNNER_EXIT, 1);
		menu_ready = 1;
	}
	if (!dynmenu_let_user_choose(&main_menu))
		return; // let dynmenu take over as runningApp for a bit

	// dynmenu is done, the user has made their choice.
	switch (dynmenu_get_user_choice(&main_menu)) { // get choice and reset for next time.
	case 0: /* new game */
		aagunner_state = AAGUNNER_NEW_GAME;
		break;
	case 1: /* quit */
		aagunner_state = AAGUNNER_EXIT;
		break;
	}
}

static void maybe_end_wave(void)
{
	static int count = 0;

	if (missiles_this_wave > wave[current_wave].missile_count) {
		count++;
		if (count > 100) {
			count = 0;
			if (current_wave < nwaves - 1)
				current_wave++;
			aagunner_state = AAGUNNER_END_WAVE;
		}
	}
}

static void aagunner_run(void)
{
	check_buttons();
	move_bullets();
	launch_missiles();
	move_missiles();
	move_sparks();
	draw_screen();
	maybe_end_wave();
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
	case AAGUNNER_MENU:
		aagunner_menu();
		break;
	case AAGUNNER_NEW_GAME:
		aagunner_new_game();
		break;
	case AAGUNNER_BEGIN_WAVE:
		aagunner_begin_wave();
		break;
	case AAGUNNER_END_WAVE:
		aagunner_end_wave();
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

