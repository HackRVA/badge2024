#include <stdio.h>
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

#define TANK_RADIUS 8
static struct point tank_points[] = {
#if 0
	{ 0, -5 },
	{ 3, 5 },
	{ -3, 5 },
	{ 0, - 5 },
#endif
	{ 5, 0 },
	{ -5, 3 },
	{ -5, -3 },
	{ 5, 0 },
};

#define MAX_TANK_SPEED (256 * 3)
#define MIN_TANK_SPEED (-128) 
#define TANK_SPEED_INCR 128
static struct tank {
	int x, y, angle, color, speed, alive;
} tank[2] = { 0 };

#define MAX_BULLETS 20
#define BULLET_LIFETIME 60
#define BULLET_SPEED (256 * 5)
static struct bullet {
	int x, y, lx, ly, vx, vy, life, shooter;
} bullet[MAX_BULLETS] = { 0 };
static int nbullets = 0;

static void tank_vs_tank_init(void)
{
	FbInit();
	FbClear();
	tank_vs_tank_state = TANK_VS_TANK_RUN;
	tank[0].color = YELLOW;
	tank[0].x = 256 * (LCD_XSIZE / 2);
	tank[0].y = 256 * 10;
	tank[0].angle = 32 * 3;
	tank[0].speed = 0;
	tank[0].alive = 1;
	tank[1].color = CYAN;
	tank[1].x = 256 * (LCD_XSIZE / 2);
	tank[1].y = 256 * (LCD_YSIZE - 10);
	tank[1].angle = 32;
	tank[1].speed = 0;
	tank[1].alive = 1;
	nbullets = 0;
}

static void add_bullet(int x, int y, int vx, int vy, int shooter)
{
	struct bullet *b;
	if (nbullets >= MAX_BULLETS)
		return;
	b = &bullet[nbullets];
	b->x = x;
	b->y = y;
	b->lx = x;
	b->ly = y;
	b->vx = vx;
	b->vy = vy;
	b->shooter = shooter;
	b->life = BULLET_LIFETIME;
	nbullets++;
}

static void remove_bullet(int n)
{
	if (n < 0 || n >= nbullets)
		return;
	if (n != nbullets - 1)
		bullet[n] = bullet[nbullets - 1];
	nbullets--;
}

static void bullet_tank_collision_detection(struct bullet *b, struct tank *t)
{
	int dx, dy, d2;

	if (t->alive <= 0) /* Can't hit a dead tank */
		return;

	dx = t->x - b->x;
	dy = t->y - b->y;
	d2 = (dx * dx) / 256 + (dy * dy) / 256;
	if (d2 / 256 < TANK_RADIUS * TANK_RADIUS)
		t->alive = -100;
}

static void move_bullet(struct bullet *b)
{
	if (b->life > 0)
		b->life--;
	b->lx = b->x;
	b->ly = b->y;
	b->x += b->vx;
	b->y += b->vy;
	if (b->x < 0 || b->y < 0 || b->x >= LCD_XSIZE * 256 || b->y >= LCD_YSIZE * 256)
		b->life = 0;
	bullet_tank_collision_detection(b, &tank[!b->shooter]);
}

static void move_bullets(void)
{
	for (int i = 0; i < nbullets; i++)
		move_bullet(&bullet[i]);
}

static void remove_dead_bullets(void)
{
	int i = 0;

	while (i < nbullets) {
		if (!bullet[i].life)
			remove_bullet(i);
		else
			i++;
	}
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
	dest->x = len * -sine(a) / 256;
	dest->y = len * -cosine(a) / 256;
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

	if (t->alive <= 0)
		return;
	rotate_points(tank_points, rotated_tank, ARRAYSIZE(tank_points), t->angle);
	x = t->x / 256;
	y = t->y / 256;
	FbDrawObject(rotated_tank, ARRAYSIZE(tank_points), t->color, x, y, 1024);
}

static void draw_bullet(struct bullet *b)
{
	FbColor(WHITE);
	FbLine(b->x / 256, b->y / 256, b->lx / 256, b->ly / 256);
}

static void draw_bullets(void)
{
	for (int i = 0; i < nbullets; i++)
		draw_bullet(&bullet[i]);
}

static void adjust_angle(int *angle, int amount)
{
	*angle += amount;
	if (*angle < 0)
		*angle += 128;
	if (*angle > 127)
		*angle -= 128;
}

static void fire_gun(struct tank *t)
{
	int vx, vy;
	vx = -BULLET_SPEED * sine(t->angle) / 256;
	vy = -BULLET_SPEED * cosine(t->angle) / 256;
	add_bullet(t->x, t->y, vx, vy, t - tank); 
}

static void check_buttons(void)
{
	int down_latches = button_down_latches();
	int r0 = button_get_rotation(0);
	int r1 = button_get_rotation(1);

	if (r0)
		adjust_angle(&tank[0].angle, -r0);
	if (r1)
		adjust_angle(&tank[1].angle, -r1);

	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		fire_gun(&tank[0]);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches)) {
		fire_gun(&tank[1]);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		tank[1].speed -= TANK_SPEED_INCR;
		if (tank[1].speed < MIN_TANK_SPEED)
			tank[1].speed = MIN_TANK_SPEED;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		tank[1].speed += TANK_SPEED_INCR;
		if (tank[1].speed > MAX_TANK_SPEED)
			tank[1].speed = MAX_TANK_SPEED;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		tank[0].speed += TANK_SPEED_INCR;
		if (tank[0].speed > MAX_TANK_SPEED)
			tank[0].speed = MAX_TANK_SPEED;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		tank[0].speed -= TANK_SPEED_INCR;
		if (tank[0].speed < MIN_TANK_SPEED)
			tank[0].speed = MIN_TANK_SPEED;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		tank_vs_tank_state = TANK_VS_TANK_EXIT;
	}
}

static void respawn_tank(struct tank *t)
{
	t->alive = 1;
}

static void move_tank(struct tank *t)
{
	int nx, ny;

	if (t->alive < 0)
		t->alive++;
	if (t->alive == 0)
		respawn_tank(t);
	nx = t->x + ((-t->speed * sine(t->angle)) / 256);
	ny = t->y + ((-t->speed * cosine(t->angle)) / 256);

	if (nx >= 0 && nx <= 256 * LCD_XSIZE && ny >= 0 && ny <= 256 * LCD_YSIZE) {
		t->x = nx;
		t->y = ny;
	} else {
		t->speed = 0;
	}
}

static void move_objects(void)
{
	move_tank(&tank[0]);
	move_tank(&tank[1]);
	move_bullets();
	remove_dead_bullets();
}

static void draw_objects(void)
{
	draw_tank(&tank[0]);
	draw_tank(&tank[1]);
	draw_bullets();
	FbSwapBuffers();
}

static void tank_vs_tank_run(void)
{
	check_buttons();
	move_objects();
	draw_objects();
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

