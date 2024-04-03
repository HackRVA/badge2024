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
#include "dynmenu.h"

/* Program states.  Initial state is MOONPATROL_INIT */
enum moonpatrol_state_t {
	MOONPATROL_INIT,
	MOONPATROL_SETUP,
	MOONPATROL_RUN,
	MOONPATROL_EXIT,
};

#define MOUNTAINS_LEN 256
#define MOUNTAINS_SEG_LEN 8
static int mountain[MOUNTAINS_LEN];

#define FOOTHILLS_LEN 512
#define FOOTHILLS_SEG_LEN 8
static int foothill[FOOTHILLS_LEN];

static enum moonpatrol_state_t moonpatrol_state = MOONPATROL_INIT;
static int screen_changed = 0;
static int num_rocks = 20;
static int num_craters = 20;
static int ground_level = 148 * 256;
static int difficulty_level = 0;
static int num_saucer_triggers = 20;
static int music_on = 1;
#define GRAVITY 64 
#define BULLET_VEL (5 * 256)
#define JUMP_VEL (3 * 256)
#define PLAYER_VEL_INC 64 

#define TERRAIN_LEN 1024
#define TERRAIN_SEG_LENGTH 16
static int terrainy[TERRAIN_LEN];
static char terrain_feature[TERRAIN_LEN];
#define FEATURE_CRATER (1 << 0)
#define FEATURE_ROCK (1 << 1)
#define FEATURE_SAUCER_TRIGGER (1 << 2)
#define FEATURE_SAUCER_TRIGGER2 (1 << 3)

#define MIN_PLAYER_VX (2 * 256)
#define MAX_PLAYER_VX (25 * 256)

/* Position of upper left of screen in game world */
static int screenx = 0;
static int screeny = 0;

static struct player {
	int x, y, vx, vy, alive;
} player = {
	0, 0, 10, 0, 1,
};

#define MAXBULLETS 10
static struct bullet {
	int x, y, vx, vy;
	int alive;
} bullet[MAXBULLETS];
static int nbullets = 0;

#define MAXSPARKS 100 
static struct spark {
	int x, y, vx, vy;
	int alive;
} spark[MAXSPARKS];
static int nsparks;

#define MAXSAUCERS 5
struct saucer {
	int x, y, vx, vy;
	int target_altitude;
	int target_xoffset;
	int alive, type;
} saucer[MAXSAUCERS];
int nsaucers;

#define whole_note (2000) 
#define half_note (whole_note / 2)
#define quarter_note (whole_note / 4)
#define dotted_quarter ((3 * whole_note) / 8)
#define eighth_note (whole_note / 8)
#define sixteenth_note (whole_note / 16)
#define thirtysecond_note (whole_note / 32)

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

static struct note moon_patrol_theme_one[] = {
        { NOTE_E3, eighth_note, },
        { NOTE_E3, eighth_note, },
        { NOTE_E4, quarter_note, },
        { NOTE_D4, eighth_note, },
        { NOTE_D4, sixteenth_note, },
        { NOTE_B3, eighth_note, },
        { NOTE_B3, eighth_note, },
        { NOTE_D4, sixteenth_note, },
        { NOTE_Ef4, sixteenth_note, },
        { NOTE_E4, eighth_note, },
};

static struct note moon_patrol_theme_four[] = {
        { NOTE_A4, eighth_note, },
        { NOTE_A4, eighth_note, },
        { NOTE_A5, quarter_note, },
        { NOTE_G5, eighth_note, },
        { NOTE_G5, sixteenth_note, },
        { NOTE_E5, eighth_note, },
        { NOTE_E5, eighth_note, },
        { NOTE_G5, sixteenth_note, },
        { NOTE_Af5, sixteenth_note, },
        { NOTE_A5, eighth_note, },
};

static struct note moon_patrol_theme_five[] = {
        { NOTE_B4, eighth_note, },
        { NOTE_B4, eighth_note, },
        { NOTE_B5, quarter_note, },
        { NOTE_A5, eighth_note, },
        { NOTE_A5, sixteenth_note, },
        { NOTE_Fs5, eighth_note, },
        { NOTE_Fs5, eighth_note, },
        { NOTE_A5, sixteenth_note, },
        { NOTE_Bf5, sixteenth_note, },
        { NOTE_B5, eighth_note, },
};

static struct tune moon_patrol_theme_1 = {
	.num_notes = ARRAYSIZE(moon_patrol_theme_one),
	.note = &moon_patrol_theme_one[0],
};

static struct tune moon_patrol_theme_4 = {
	.num_notes = ARRAYSIZE(moon_patrol_theme_four),
	.note = &moon_patrol_theme_four[0],
};

static struct tune moon_patrol_theme_5 = {
	.num_notes = ARRAYSIZE(moon_patrol_theme_five),
	.note = &moon_patrol_theme_five[0],
};

static struct point saucer_points[] = {
	{ -62, -1 },
	{ -36, -17 },
	{ 1, -16 },
	{ 42, -16 },
	{ 63, -1 },
	{ 35, 9 },
	{ 23, 0 },
	{ -21, 1 },
	{ -33, 9 },
	{ -62, 2 },
	{ -128, -128 },
	{ -27, -18 },
	{ -26, -35 },
	{ -13, -44 },
	{ 15, -44 },
	{ 28, -32 },
	{ 29, -19 },
};

static struct point saucer2_points[] = {
	{ -61, 0 },
	{ -42, -10 },
	{ 50, -11 },
	{ 62, -1 },
	{ 55, 13 },
	{ 40, 19 },
	{ 25, 25 },
	{ -25, 24 },
	{ -53, 10 },
	{ -60, -1 },
	{ -128, -128 },
	{ -17, 25 },
	{ -14, 32 },
	{ 15, 31 },
	{ 18, 23 },
	{ -128, -128 },
	{ -31, -11 },
	{ -14, -31 },
	{ -3, -36 },
	{ 5, -36 },
	{ 20, -29 },
	{ 37, -11 },
	{ -128, -128 },
	{ -19, -18 },
	{ -8, -30 },
};

static void init_player(void);

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
	if (player.alive <= 0) {
		player.alive++;
		if (player.alive == 0)
			init_player();
	}
}

static void add_saucer(int type)
{
	static unsigned int r = 0x5a5a5a5a;

	if (nsaucers >= MAXSAUCERS)
		return;

	saucer[nsaucers].x = player.x + xorshift(&r) % (50 * 256);
	saucer[nsaucers].y = -(20 * 256);
	saucer[nsaucers].vx = player.vx;
	saucer[nsaucers].vy = 128;
	saucer[nsaucers].target_altitude = 60 * 256;
	saucer[nsaucers].target_xoffset = 70 * 256;
	saucer[nsaucers].alive = 400;
	saucer[nsaucers].type = type;
	nsaucers++;
	printf("Added saucer!\n");
}

static void add_saucers(int count, int type)
{
	count = 1; /* temporary */
	for (int i = 0; i < count; i++)
		add_saucer(type);
}

static void draw_saucer(int i)
{
	if (i >= nsaucers)
		return;
	int x, y;

	x = (saucer[i].x - screenx) / 256;
	y = saucer[i].y / 256;
	if (saucer[i].type == 0)
		FbDrawObject(saucer_points, ARRAYSIZE(saucer_points), YELLOW, x, y, 128);
	else
		FbDrawObject(saucer2_points, ARRAYSIZE(saucer2_points), x11_lime_green, x, y, 128);
}

static void draw_saucers(void)
{
	for (int i = 0; i < nsaucers; i++)
		draw_saucer(i);
}

static void remove_saucer(int i)
{
	if (i != nsaucers - 1)
		saucer[i] = saucer[nsaucers - 1];
	nsaucers--;
}

static void move_saucer(int i)
{
	unsigned int r = 0x5a5a5a5a;
	int dx, dy;
	saucer[i].x += saucer[i].vx;
	saucer[i].y += saucer[i].vy;
	if (saucer[i].alive > 0)
		saucer[i].alive--;
	dx = (player.x + saucer[i].target_xoffset) - saucer[i].x;
	if ((dx < 0 && dx > -(10 * 256)) || (dx > 0 && dx < (10 * 256))) {
		/* close enough, choose a new target xoffset */
		if (saucer[i].target_xoffset > 0)
			saucer[i].target_xoffset = -(10 * 256);
		else
			saucer[i].target_xoffset = (60 * 256);
	}
	dy = saucer[i].target_altitude - saucer[i].y;
	if ((dy < 0 && dy > -(1 * 256)) || (dy > 0 && dy < (1 * 256))) {
		/* choose a new target altitude */ 
		saucer[i].target_altitude = 40 + (xorshift(&r) % 20);
	}
	dy = saucer[i].target_altitude - saucer[i].y;
	if (dy < 0 && saucer[i].vy > -(1 * 256))
		saucer[i].vy -= 20;
	else if (dy > 0 && saucer[i].vy < (1 * 256))
		saucer[i].vy += 20;
	if (dx < 0 && saucer[i].vx > player.vx - (5 * 256))
		saucer[i].vx -= 20;
	if (dx > 0 && saucer[i].vx < player.vx + (5 * 256))
		saucer[i].vx += 20;
}

static void move_saucers(void)
{
	for (int i = 0; i < nsaucers;) {
		move_saucer(i);
		if (saucer[i].alive)
			i++;
		else
			remove_saucer(i);
	}
}

static void add_explosion(int x, int y, int count, int life);

static void move_bullet(int n)
{
	bullet[n].x += bullet[n].vx;
	bullet[n].y += bullet[n].vy;
	if (bullet[n].x - screenx >= (LCD_XSIZE * 256))
		bullet[n].alive = 0;
	if (bullet[n].y <= 0)
		bullet[n].alive = 0;
	int bi = ((bullet[n].x / 256) / TERRAIN_SEG_LENGTH);
	if (terrain_feature[bi] & FEATURE_ROCK) {
		int dy = terrainy[bi] - bullet[n].y;
		dy = dy / 256;
		if (dy < 0)
			dy = -dy;
		if (dy < 16) {
			terrain_feature[bi] &= ~FEATURE_ROCK;
			printf("blasted rock! dy = %d\n", dy);
			add_explosion(bullet[n].x, bullet[n].y, 40, 50);
			bullet[n].alive = 0;
		}
	}
	/* Check to see if we killed any saucers ... */
	for (int i = 0; i < nsaucers; i++) {
		int dx, dy;

		dx = (bullet[n].x - saucer[i].x) / 256;
		dy = (bullet[n].y - saucer[i].y) / 256;
		dx *= dx;
		dy *= dy;
		if (dx + dy < 25) { /* < 5 pixels */
			add_explosion(saucer[i].x, saucer[i].y, 20, 50);
			remove_saucer(i);
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

static unsigned int foothills_state = 0xa5a5a5a5;

static void generate_foothill(int foothill[], int y1, int y2, int start, int end)
{
	foothill[start] = y1;
	foothill[end] = y2;
	int mid = ((end - start) / 2) + start;
	if (mid < end && mid > start) {
		int dy = y1 - y2;
		if (dy < 0)
			dy = -dy;
		int r = (xorshift(&foothills_state) % 200) - 100; /* +/- 100 */
		int d = (dy * r) / 250; /* up to 40% deviation from mid point */
		int y3 = ((y1 + y2) / 2) + d;
		foothill[mid] = y3;
		generate_foothill(foothill, y1, y3, start, mid);
		generate_foothill(foothill, y3, y2, mid, end);
	}
}

static void generate_hills(int foothill[], int len, int low, int high, int interval)
{
	int i = 0;
	int a = low;
	int b;
	do {
		int i2 = i + interval;
		if (i2 > len - 1)
			i2 = len - 1;
		if (a == low)
			b = high;
		else
			b = low;
		generate_foothill(foothill, a, b, i, i2);
		i = i2;
		a = b;
		if (i == len - 1)
			break;
	} while (1);
}

static void generate_terrain(void)
{
	static unsigned int rstate = 0xa5a5a5a5;

	for (int i = 0; i < TERRAIN_LEN; i++) {
		terrainy[i] = LCD_YSIZE - 10 - (xorshift(&rstate) % 8); /* TODO: something better */
		terrainy[i] = terrainy[i] * 256;
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
	for (int i = 0; i < num_saucer_triggers; i++) {
		int x = 10 + (xorshift(&rstate) % (TERRAIN_LEN - 11));
		terrain_feature[x] |= FEATURE_SAUCER_TRIGGER;
		printf("saucer trigger at %d\n", x);
	}
	for (int i = 0; i < num_saucer_triggers; i++) {
		int x = 10 + (xorshift(&rstate) % (TERRAIN_LEN - 11));
		terrain_feature[x] |= FEATURE_SAUCER_TRIGGER2;
		printf("saucer2 trigger at %d\n", x);
	}
}

static void init_player(void)
{
	player.x = 0;
	player.y = 148 * 256;
	player.vx = 0;
	player.vy = 0;
	player.alive = 1;
}

static void play_theme(void *cookie)
{
	intptr_t measure = (intptr_t) cookie;

	switch (measure % 12) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 6:
		case 7:
			play_tune(&moon_patrol_theme_1, play_theme, (void *) (measure + 1));
			break;
		case 4:
		case 5:
			play_tune(&moon_patrol_theme_4, play_theme, (void *) (measure + 1));
			break;
		case 8:
			play_tune(&moon_patrol_theme_5, play_theme, (void *) (measure + 1));
			break;
		case 9:
			play_tune(&moon_patrol_theme_4, play_theme, (void *) (measure + 1));
			break;
		case 10:
			play_tune(&moon_patrol_theme_1, play_theme, (void *) (measure + 1));
			break;
		case 11:
			play_tune(&moon_patrol_theme_1, play_theme, (void *) (measure + 1));
			break;
	}
}

static void moonpatrol_init(void)
{
	FbInit();
	FbClear();
	generate_terrain();
	generate_hills(foothill, FOOTHILLS_LEN, 120 * 256, 80 * 256, 20);
	generate_hills(mountain, MOUNTAINS_LEN, 60 * 256, 20 * 256, 10);
	moonpatrol_state = MOONPATROL_SETUP;
	screen_changed = 1;
	init_player();
	memset(bullet, 0, sizeof(bullet));
	play_theme((void *) 0);
	nbullets = 0;
}

static void moonpatrol_setup(void)
{
	static struct dynmenu setup_menu;
	static struct dynmenu_item setup_item[6];
	static int menu_ready = 0;
        char *level[] = { "EASY", "MEDIUM", "HARD" };

        if (!menu_ready) {
                dynmenu_init(&setup_menu, setup_item, 6);
                dynmenu_clear(&setup_menu);
                strcpy(setup_menu.title, "LUNAR RESCUE");
                dynmenu_add_item(&setup_menu, "EASY <==", MOONPATROL_SETUP, 0);
                dynmenu_add_item(&setup_menu, "MEDIUM", MOONPATROL_SETUP, 1);
                dynmenu_add_item(&setup_menu, "HARD", MOONPATROL_SETUP, 2);
		dynmenu_add_item(&setup_menu, "MUSIC: ON", MOONPATROL_SETUP, 3);
                dynmenu_add_item(&setup_menu, "PLAY NOW", MOONPATROL_SETUP, 4);
                dynmenu_add_item(&setup_menu, "QUIT", MOONPATROL_SETUP, 5);
                menu_ready = 1;
        }
	dynmenu_draw(&setup_menu);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
		dynmenu_change_current_selection(&setup_menu, 1);
	else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
		dynmenu_change_current_selection(&setup_menu, -1);
	else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {

		int c = setup_menu.current_item;
		switch(c) {
		case 0:
		case 1:
		case 2:
			difficulty_level = c;
			for (int i = 0; i < 3; i++)
				strcpy(setup_menu.item[i].text, level[i]);
			strcat(setup_menu.item[c].text, " <==");
			break;
		case 3:
			if (music_on) {
				music_on = 0;
				stop_tune();
				strcpy(setup_menu.item[3].text, "MUSIC: OFF");
			} else {
				music_on = 1;
				play_theme((void *) 0);
				strcpy(setup_menu.item[3].text, "MUSIC: ON");
			}
			break;
		case 4:
			moonpatrol_state = MOONPATROL_RUN;
			break;
		case 5:
			moonpatrol_state = MOONPATROL_EXIT;
			break;
		}
	}
}

static void moonpatrol_shoot(void)
{
	add_bullet(player.x, player.y, player.vx, -BULLET_VEL);
	add_bullet(player.x, player.y - (5 * 256), player.vx + BULLET_VEL, 0);
}

static void player_maybe_jump(void)
{
	int playeri = ((player.x / 256) / TERRAIN_SEG_LENGTH);
	int dy = player.y - terrainy[playeri];
	printf("py = %d, ty = %d, dy = %d\n", player.y, terrainy[playeri], dy);
	if (-dy < (3 * 256)) { /* prevent mid-air jumping */
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
	int i = ((screenx / 256) / TERRAIN_SEG_LENGTH) - 2;
	int x1, y1, x2, y2, i2;

	x1 = i * TERRAIN_SEG_LENGTH - (screenx / 256);
	if (i < 0)
		i += TERRAIN_LEN;
	x2 = x1 + TERRAIN_SEG_LENGTH;
	y1 = terrainy[i] / 256;
	i2 = i + 1;
	if (i2 >= TERRAIN_LEN)
		i2 = i2 - TERRAIN_LEN;
	y2 = terrainy[i2] / 256;

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
		y2 = terrainy[i2] / 256;
		x1 += TERRAIN_SEG_LENGTH;
		x2 += TERRAIN_SEG_LENGTH;
		if (x1 >= LCD_XSIZE)
			break;

		if (x1 < ((player.x - screenx) / 256) && x2 > ((player.x - screenx) / 256))
			ground_level = ((y1 + y2) / 2) * 256;
	} while(1);
}

static void draw_hills(int hill[], int len, int seglen, int factor, int color)
{
	int i = (((screenx / factor) / 256) / seglen) - 2;
	int x1, y1, x2, y2, i2;

	x1 = i * seglen - ((screenx / factor) / 256);
	if (i < 0)
		i += len;
	x2 = x1 + seglen;
	y1 = hill[i] / 256;
	i2 = i + 1;
	if (i2 >= len)
		i2 = i2 - len;
	y2 = hill[i2] / 256;

	FbColor(color);
	do {
		if (FbOnScreen(x1, y1) || FbOnScreen(x2, y2)) {
			FbClippedLine(x1, y1, x2, y2);
		}
		i2 = (i2 + 1) % len;
		i = (i + 1) % len;
		y1 = y2;
		if (i2 >= len)
			i2 = i2 - len;
		y2 = hill[i2] / 256;
		x1 += seglen;
		x2 += seglen;
		if (x1 >= LCD_XSIZE)
			break;
	} while(1);
}

static void draw_player(void)
{
	int x = (player.x - screenx) / 256;
	int y = (player.y - screeny) / 256;

	if (player.alive < 0)
		return;

	FbColor(MAGENTA);
	FbMove(x - 5, y - 5);
	FbRectangle(10, 5);

	screenx = player.x - (10 * 256);
}

static void draw_spark(int i)
{
	int x, y;

	x = (spark[i].x - screenx) / 256;
	y = (spark[i].y - screeny) / 256;
	if (FbOnScreen(x, y)) {
		FbColor(YELLOW);
		FbPoint(x, y);
	}
}

static void draw_sparks(void)
{
	for (int i = 0; i < nsparks; i++)
		draw_spark(i);
}

static void move_spark(int i)
{
	spark[i].x += spark[i].vx;
	spark[i].y += spark[i].vy;
	spark[i].vy += (GRAVITY / 2); /* / 2 because it looks better */
	if (spark[i].alive > 0)
		spark[i].alive--;
}

static void delete_spark(int i)
{
	if (i >= nsparks)
		return;
	if (i < nsparks)
		spark[i] = spark[nsparks - 1];
	nsparks--;
}

static void move_sparks(void)
{
	int i = 0;

	while (i < nsparks) {
		move_spark(i);
		if (!spark[i].alive)
			delete_spark(i);
		else
			i++;
	}

}

static void add_spark(int x, int y, int vx, int vy, int lifetime)
{
	if (nsparks >= MAXSPARKS)
		return;
	spark[nsparks].x = x;
	spark[nsparks].y = y;
	spark[nsparks].vx = vx;
	spark[nsparks].vy = vy;
	spark[nsparks].alive = lifetime;
	nsparks++;
}

static void add_explosion(int x, int y, int count, int life)
{
	static unsigned int xorshift_state = 0xa5a5a5a5;
        for (int i = 0; i < count; i++) {
		int vx, vy;
                vx = xorshift(&xorshift_state) % (5 * 256);
                vy = xorshift(&xorshift_state) % (5 * 256);
		vx = vx - ((5 * 256) / 2);
		vy = vy - ((5 * 256) / 2);
		add_spark(x, y, vx, vy, xorshift(&xorshift_state) % life);
        }
}

static void draw_bullet(int i)
{
	int x1, y1, x2, y2;

	FbColor(WHITE);
	if (bullet[i].vy != 0) {
		x1 = (bullet[i].x - screenx) / 256;
		y1 = (bullet[i].y + bullet[i].vy) / 256;
		x2 = x1;
		y2 = bullet[i].y / 256;
	} else {
		x1 = (bullet[i].x - screenx - bullet[i].vx) / 256;
		y1 = bullet[i].y / 256;
		x2 = (bullet[i].x - screenx) / 256;
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
	draw_hills(foothill, FOOTHILLS_LEN, FOOTHILLS_SEG_LEN, 2, x11_lime_green);
	draw_hills(mountain, MOUNTAINS_LEN, MOUNTAINS_SEG_LEN, 4, WHITE);
	draw_player();
	draw_saucers();
	draw_bullets();
	draw_sparks();
	FbSwapBuffers();
	screen_changed = 0;
}

static void kill_player(void)
{
	player.alive = -50;
	player.vx = 0;
	player.vy = 0;
	nsaucers = 0;
	add_explosion(player.x, player.y, 50, 100); 
}

static void moonpatrol_run(void)
{
	check_buttons();
	move_player();
	move_saucers();
	move_bullets();
	move_sparks();
	draw_screen();
	screen_changed = 1;

	int playeri = ((player.x / 256) / TERRAIN_SEG_LENGTH);
	if (terrain_feature[playeri] & FEATURE_ROCK) {
		if (player.vy == 0) {
			int dy = (player.y - terrainy[playeri]) / 256;
			if (dy < 0)
				dy = -dy;
			if (dy < 3 && player.alive > 0) {
				printf("hit rock!\n");
				kill_player();
			}
		}
	}
	if (terrain_feature[playeri] & FEATURE_CRATER) {
		if (player.vy == 0) {
			int dy = (player.y - terrainy[playeri]) / 256;
			if (dy < 0)
				dy = -dy;
			if (dy < 3 && player.alive > 0) {
				printf("hit crater!\n");
				kill_player();
			}
		}
	}
	if (terrain_feature[playeri] & FEATURE_SAUCER_TRIGGER) {
		add_saucers((playeri % 3) + 1, 0); /* add 1 to 3 saucers */
		terrain_feature[playeri] &= ~FEATURE_SAUCER_TRIGGER; /* prevent multiple triggers */
	}
	if (terrain_feature[playeri] & FEATURE_SAUCER_TRIGGER2) {
		add_saucers((playeri % 3) + 1, 1); /* add 1 to 3 saucers */
		terrain_feature[playeri] &= ~FEATURE_SAUCER_TRIGGER2; /* prevent multiple triggers */
	}
}

static void moonpatrol_exit(void)
{
	moonpatrol_state = MOONPATROL_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void moonpatrol_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (moonpatrol_state) {
	case MOONPATROL_INIT:
		moonpatrol_init();
		break;
	case MOONPATROL_SETUP:
		moonpatrol_setup();
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

