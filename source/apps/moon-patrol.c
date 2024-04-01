#include <stdio.h>
#include <string.h>

#include "colors.h"
#include "x11_colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "moon-patrol.h"
#include "xorshift.h"
#include "audio.h"
#include "music.h"

/* Program states.  Initial state is MOONPATROL_INIT */
enum moonpatrol_state_t {
	MOONPATROL_INIT,
	MOONPATROL_RUN,
	MOONPATROL_EXIT,
};

static enum moonpatrol_state_t moonpatrol_state = MOONPATROL_INIT;
static int screen_changed = 0;
static int num_rocks = 20;
static int num_craters = 20;
static int ground_level = 148 << 8;
#define GRAVITY 64 
#define BULLET_VEL (5 << 8)
#define JUMP_VEL (3 << 8)
#define PLAYER_VEL_INC 64 

#define TERRAIN_LEN 1024
#define TERRAIN_SEG_LENGTH 16
static int terrainy[TERRAIN_LEN];
static char terrain_feature[TERRAIN_LEN];
#define FEATURE_CRATER (1 << 0)
#define FEATURE_ROCK (1 << 1)

#define MIN_PLAYER_VX (2 << 8)
#define MAX_PLAYER_VX (25 << 8)

/* Position of upper left of screen in game world */
static int screenx = 0;
static int screeny = 0;

static struct player {
	int x, y, vx, vy;
} player = {
	0, 0, 10, 0,
};

#define MAXBULLETS 10
static struct bullet {
	int x, y, vx, vy;
	int alive;
} bullet[MAXBULLETS];
static int nbullets = 0;

#define whole_note 3200
#define half_note 1600
#define quarter_note 800
#define dotted_quarter 1200
#define eighth_note 400
#define sixteenth_note 200
#define thirtysecond_note 100

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

static struct note moon_patrol_theme_notes[] = {
        { NOTE_E3, eighth_note, },
        { NOTE_E3, eighth_note, },
        { NOTE_E4, quarter_note, },
        { NOTE_E4, eighth_note, },
        { NOTE_D4, eighth_note, },
        { NOTE_D4, eighth_note, },
        { NOTE_B4, eighth_note, },
};

static struct tune moon_patrol_theme = {
	.num_notes = ARRAYSIZE(moon_patrol_theme_notes),
	.note = &moon_patrol_theme_notes[0],
};

static void move_player(void)
{
	player.x += player.vx;
	player.y += player.vy;
	if (player.y >= ground_level) {
		player.y = ground_level;
		player.vy = 0;
	} else {
		player.vy += GRAVITY;
	}
}

static void move_bullet(int n)
{
	bullet[n].x += bullet[n].vx;
	bullet[n].y += bullet[n].vy;
	if (bullet[n].x - screenx >= (LCD_XSIZE << 8))
		bullet[n].alive = 0;
	if (bullet[n].y <= 0)
		bullet[n].alive = 0;
	int bi = ((bullet[n].x >> 8) / TERRAIN_SEG_LENGTH);
	if (terrain_feature[bi] & FEATURE_ROCK) {
		int dy = terrainy[bi] - bullet[n].y;
		dy = dy >> 8;
		if (dy < 0)
			dy = -dy;
		if (dy < 16) {
			terrain_feature[bi] &= ~FEATURE_ROCK;
			printf("blasted rock! dy = %d\n", dy);
		}
	}
}

static void remove_bullet(int n)
{
	if (n >= nbullets)
		return;
	if (n < nbullets - 1)
		bullet[n] = bullet[nbullets - 1];
	nbullets--;
}

static void add_bullet(int x, int y, int vx, int vy)
{
	if (nbullets >= MAXBULLETS)
		return;

	bullet[nbullets].x = x;
	bullet[nbullets].y = y;
	bullet[nbullets].vx = vx;
	bullet[nbullets].vy = vy;
	bullet[nbullets].alive = 1;
	nbullets++;
}

static void move_bullets(void)
{
	int i = 0;
	while (i < nbullets) {
		move_bullet(i);
		if (!bullet[i].alive)
			remove_bullet(i);
		else
			i++;
	}
}

static void generate_terrain(void)
{
	static unsigned int rstate = 0xa5a5a5a5;

	for (int i = 0; i < TERRAIN_LEN; i++) {
		terrainy[i] = LCD_YSIZE - 10 - (xorshift(&rstate) % 8); /* TODO: something better */
		terrainy[i] = terrainy[i] << 8;
	}
	terrainy[0] = terrainy[TERRAIN_LEN - 1];
	memset(terrain_feature, 0, sizeof(terrain_feature));
	for (int i = 0; i < num_craters; i++) {
		int x = 10 + (xorshift(&rstate) % (TERRAIN_LEN - 11));
		terrain_feature[x] |= FEATURE_CRATER;
		printf("Crater at %d\n", x);
	}
	for (int i = 0; i < num_rocks; i++) {
		int x = 10 + (xorshift(&rstate) % (TERRAIN_LEN - 11));
		if (!(terrain_feature[x] & FEATURE_CRATER)) {
			terrain_feature[x] |= FEATURE_ROCK;
			printf("rock at %d\n", x);
		}
	}
}

static void moonpatrol_init(void)
{
	FbInit();
	FbClear();
	generate_terrain();
	moonpatrol_state = MOONPATROL_RUN;
	screen_changed = 1;
	player.x = 0;
	player.y = 148 << 8;
	player.vx = 0;
	player.vy = 0;
	memset(bullet, 0, sizeof(bullet));
	nbullets = 0;
	// play_tune(&moon_patrol_theme, NULL);
}

static void moonpatrol_shoot(void)
{
	add_bullet(player.x, player.y, player.vx, -BULLET_VEL);
	add_bullet(player.x, player.y - (5 << 8), player.vx + BULLET_VEL, 0);
}

static void player_maybe_jump(void)
{
	int playeri = ((player.x >> 8) / TERRAIN_SEG_LENGTH);
	int dy = player.y - terrainy[playeri];
	printf("py = %d, ty = %d, dy = %d\n", player.y, terrainy[playeri], dy);
	if (-dy < (3 << 8)) { /* prevent mid-air jumping */
		player.vy = -JUMP_VEL;
	}
}

static void check_buttons(void)
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		player.vx -= PLAYER_VEL_INC;
		if (player.vx < MIN_PLAYER_VX)
			player.vx = MIN_PLAYER_VX;
		screenx--;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		player.vx += PLAYER_VEL_INC;
		if (player.vx > MAX_PLAYER_VX)
			player.vx = MAX_PLAYER_VX;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		player_maybe_jump();
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		moonpatrol_shoot();
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		moonpatrol_state = MOONPATROL_EXIT;
	}
}

static void draw_terrain(void)
{
	int i = ((screenx >> 8) / TERRAIN_SEG_LENGTH) - 2;
	int x1, y1, x2, y2, i2;

	x1 = i * TERRAIN_SEG_LENGTH - (screenx >> 8);
	if (i < 0)
		i += TERRAIN_LEN;
	x2 = x1 + TERRAIN_SEG_LENGTH;
	y1 = terrainy[i] >> 8;
	i2 = i + 1;
	if (i2 >= TERRAIN_LEN)
		i2 = i2 - TERRAIN_LEN;
	y2 = terrainy[i2] >> 8;

	FbColor(WHITE);
	do {
		if (FbOnScreen(x1, y1) || FbOnScreen(x2, y2)) {
			if (terrain_feature[i] & FEATURE_ROCK) {
				FbColor(x11_orange);
				FbClippedLine(x1, y1, x1 + (x2 - x1) / 2, y1 - 10);
				FbClippedLine(x1 + (x2 - x1) / 2, y1 - 10, x2, y2);
				FbColor(WHITE);
				FbClippedLine(x1, y1, x2, y2);
			} else if (terrain_feature[i] & FEATURE_CRATER) {
				FbClippedLine(x1, y1, x1 + (x2 - x1) / 3, 159);
				FbClippedLine(x1 + (x2 - x1) / 3, 159, x1 + 2 * (x2 - x1) / 3, 159);
				FbClippedLine(x1 + 2 * (x2 - x1) / 3, 159, x2, y2);
			} else {
				FbClippedLine(x1, y1, x2, y2);
			}
		}
		i2 = (i2 + 1) % TERRAIN_LEN;
		i = (i + 1) % TERRAIN_LEN;
		y1 = y2;
		if (i2 >= TERRAIN_LEN)
			i2 = i2 - TERRAIN_LEN;
		y2 = terrainy[i2] >> 8;
		x1 += TERRAIN_SEG_LENGTH;
		x2 += TERRAIN_SEG_LENGTH;
		if (x1 >= LCD_XSIZE)
			break;

		if (x1 < ((player.x - screenx) >> 8) && x2 > ((player.x - screenx) >> 8))
			ground_level = ((y1 + y2) / 2) << 8;
	} while(1);
}

static void draw_player(void)
{
	int x = (player.x - screenx) >> 8;
	int y = (player.y - screeny) >> 8;

	FbColor(MAGENTA);
	FbMove(x - 5, y - 5);
	FbRectangle(10, 5);

	screenx = player.x - (10 << 8);
}

static void draw_bullet(int i)
{
	int x1, y1, x2, y2;

	FbColor(WHITE);
	if (bullet[i].vy != 0) {
		x1 = (bullet[i].x - screenx) >> 8;
		y1 = (bullet[i].y + bullet[i].vy) >> 8;
		x2 = x1;
		y2 = bullet[i].y >> 8;
	} else {
		x1 = (bullet[i].x - screenx - bullet[i].vx) >> 8;
		y1 = bullet[i].y >> 8;
		x2 = (bullet[i].x - screenx) >> 8;
		y2 = y1;
	}
	FbClippedLine(x1, y1, x2, y2);
}

static void draw_bullets(void)
{
	for (int i = 0; i < nbullets; i++)
		draw_bullet(i);
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	draw_terrain();
	draw_player();
	draw_bullets();
	FbSwapBuffers();
	screen_changed = 0;
}

static void moonpatrol_run(void)
{
	check_buttons();
	move_player();
	move_bullets();
	draw_screen();
	screen_changed = 1;

	int playeri = ((player.x >> 8) / TERRAIN_SEG_LENGTH);
	if (terrain_feature[playeri] & FEATURE_ROCK) {
		if (player.vy == 0) {
			int dy = (player.y - terrainy[playeri]) >> 8;
			if (dy < 0)
				dy = -dy;
			if (dy < 3)
				printf("hit rock!\n");
		}
	}
	if (terrain_feature[playeri] & FEATURE_CRATER) {
		if (player.vy == 0) {
			int dy = (player.y - terrainy[playeri]) >> 8;
			if (dy < 0)
				dy = -dy;
			if (dy < 3)
				printf("hit crater!\n");
		}
	}
}

static void moonpatrol_exit(void)
{
	moonpatrol_state = MOONPATROL_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void moonpatrol_cb(__attribute__((unused)) struct menu_t *m)
{
	(void) moon_patrol_theme;
	switch (moonpatrol_state) {
	case MOONPATROL_INIT:
		moonpatrol_init();
		break;
	case MOONPATROL_RUN:
		moonpatrol_run();
		break;
	case MOONPATROL_EXIT:
		moonpatrol_exit();
		break;
	default:
		break;
	}
}

