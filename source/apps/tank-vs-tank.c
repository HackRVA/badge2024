#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "trig.h"
#include "fxp_sqrt.h"
#include "random.h"
#include "xorshift.h"
#include "dynmenu.h"
#include "badge.h"

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Program states.  Initial state is TANK_VS_TANK_INIT */
enum tank_vs_tank_state_t {
	TANK_VS_TANK_INIT,
	TANK_VS_TANK_INTRO,
	TANK_VS_TANK_RUN,
	TANK_VS_TANK_CONFIRM_EXIT,
	TANK_VS_TANK_EXIT,
};

static enum tank_vs_tank_state_t tank_vs_tank_state = TANK_VS_TANK_INIT;

static struct dynmenu quit_menu;
static struct dynmenu_item quit_menu_item[2];

#define TANK_RADIUS 8
static struct point tank_points[] = {
	/* main body */
	{ -6, -4 },
	{ 6, -4},
	{ 6, 4 },
	{ -6, 4 },
	{ -6, -4 },

	/* turret */
	{ -128, -128 },
	{ -3, -2 },
	{ 3, -2},
	{ 3, 2 },
	{ -3, 2 },
	{ -3, -2 },

	/* gun barrel */
	{ 0, 0 },
	{ 8, 0 },
};

#define MAX_TANK_SPEED (256 * 3)
#define MIN_TANK_SPEED (-128) 
#define TANK_SPEED_INCR 128
static struct tank {
	int x, y, angle, color, speed, alive, score;
} tank[2] = { 0 };

#define MAX_BULLETS 20
#define BULLET_LIFETIME 60
#define BULLET_SPEED (256 * 5)
static struct bullet {
	int x, y, lx, ly, vx, vy, life, shooter;
} bullet[MAX_BULLETS] = { 0 };
static int nbullets = 0;

#define MAX_SPARKS 100
#define MIN_SPARK_LIFETIME 10
static struct spark {
	int x, y, vx, vy, life;
} spark[MAX_SPARKS] = { 0 };
static int nsparks;

/* Obstacles are just rectangles that are impassable to tanks and bullets */
static struct obstacle {
	int x, y, w, h;
} obstacle[] = {
	{ 15, 15, 5, 30 },
	{ 128 - 20, 15, 5, 30 },
	{ 15, 115, 5, 30 },
	{ 128 - 20, 115, 5, 30 },
	{ 30, 73, 70, 5 },
};
static int nobstacles = ARRAYSIZE(obstacle);

static void tank_vs_tank_init(void)
{
	FbInit();
	FbClear();
	tank[0].color = YELLOW;
	tank[0].x = 256 * (LCD_XSIZE / 2);
	tank[0].y = 256 * 10;
	tank[0].angle = 32 * 3;
	tank[0].speed = 0;
	tank[0].alive = 1;
	tank[0].score = 0;
	tank[1].color = CYAN;
	tank[1].x = 256 * (LCD_XSIZE / 2);
	tank[1].y = 256 * (LCD_YSIZE - 10);
	tank[1].angle = 32;
	tank[1].speed = 0;
	tank[1].alive = 1;
	tank[1].score = 0;
	nsparks = 0;
	nbullets = 0;

	dynmenu_init(&quit_menu, quit_menu_item, ARRAYSIZE(quit_menu_item));
	dynmenu_clear(&quit_menu);
	strcpy(quit_menu.title, "REALLY QUIT?");
	dynmenu_add_item(&quit_menu, "NO", TANK_VS_TANK_RUN, 0);
	dynmenu_add_item(&quit_menu, "YES", TANK_VS_TANK_EXIT, 1);

	tank_vs_tank_state = TANK_VS_TANK_INTRO;
}

static void tank_vs_tank_intro(void)
{
	FbClear();
	FbColor(WHITE);
	FbBackgroundColor(BLACK);

	FbWriteString(
		"THIS IS A TWO\n"
		"PLAYER GAME.\n\n"
		"RIGHT PLAYER:\n"
		"USE ROTARY KNOB\n"
		"TO STEER & SHOOT\n"
		"A IS FORWARD, B\n"
		"IS REVERSE.\n\n"
		"LEFT PLAYER:\n"
		"USE ROTARY KNOB\n"
		"TO STEER & SHOOT\n"
		"RIGHT DPAD IS\n"
		"FORWARD, LEFT IS\n"
		"REVERSE\n\n"
		"DPAD UP TO QUIT\n");
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches) ||
#if BADGE_HAS_ROTARY_SWITCHES
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
#endif
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
		tank_vs_tank_state = TANK_VS_TANK_RUN;
}

static void add_spark(int x, int y, int vx, int vy)
{
	struct spark *s;
	if (nsparks >= MAX_SPARKS)
		return;
	s = &spark[nsparks];
	s->x = x;
	s->y = y;
	s->vx = vx;
	s->vy = vy;
	s->life = MIN_SPARK_LIFETIME + (nsparks % 20);
	nsparks++;
}

/* return a random int between 0 and n - 1 */
static int random_num(int n)
{
	int x;
	static unsigned int state = 0;

	assert(n != 0);
	if (state == 0)
		random_insecure_bytes((uint8_t *) &state, sizeof(state));
	x = xorshift(&state);
	if (x < 0)
		x = -x;
	return x % n;
}

static void add_random_spark(int x, int y)
{
	int angle, speed, vx, vy;

	angle = random_num(128);
	speed = random_num(256 * 5) + 256 * 3;
	vx = (-sine(angle) * speed) / 256;
	vy = (-cosine(angle) * speed) / 256;
	add_spark(x, y, vx, vy);
}

static void add_sparks(int x, int y, int count)
{
	for (int i = 0; i < count; i++)
		add_random_spark(x, y);
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

static void remove_spark(int n)
{
	if (n < 0 || n >= nsparks)
		return;
	if (n != nsparks - 1)
		spark[n] = spark[nsparks - 1];
	nsparks--;
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
	if (d2 / 256 < TANK_RADIUS * TANK_RADIUS) {
		b->life = 0;
		t->alive = -100;
		add_sparks(t->x, t->y, 50);
		tank[!(t - tank)].score++;
	}
}

static int point_obstacle_collision(int x, int y, struct obstacle *o)
{
	if (x < o->x)
		return 0;
	if (y < o->y)
		return 0;
	if (x >= o->x + o->w)
		return 0;
	if (y >= o->y + o->h)
		return 0;
	return 1;
}

static int tank_obstacle_collision(int x, int y)
{
	int sx, sy;

	sx = x / 256;
	sy = y / 256;
	for (int i = 0; i < nobstacles; i++)
		if (point_obstacle_collision(sx, sy, &obstacle[i])) {
			return 1;
		}
	return 0;
}

static int tank_tank_collision(struct tank *t, int x, int y)
{
	int tx, ty, ex, ey, dx, dy, d2;
	struct tank *enemy_tank = &tank[!(t - tank)];

	if (enemy_tank->alive <= 0)
		return 0;

	tx = x / 256;
	ty = y / 256;
	ex = enemy_tank->x / 256;
	ey = enemy_tank->y / 256;
	dx = tx - ex;
	dy = ty - ey;
	d2 = dx * dx + dy * dy;
	return d2 < TANK_RADIUS * TANK_RADIUS;
}

static void bullet_obstacle_collision_detection(struct bullet *b)
{
	for (int i = 0; i < nobstacles; i++) {
		int bx, by;
		bx = b->x / 256;
		by = b->y / 256;
		if (point_obstacle_collision(bx, by, &obstacle[i])) {
			b->life = 0;
			add_sparks(b->x, b->y, 5);
			break;
		}
	}
}

static void move_spark(struct spark *s)
{
	if (s->life > 0)
		s->life--;
	s->x += s->vx;
	s->y += s->vy;
	if (s->x < 0 || s->y < 0 || s->x >= LCD_XSIZE * 256 || s->y >= LCD_YSIZE * 256)
		s->life = 0;
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
	bullet_obstacle_collision_detection(b);
}

static void move_sparks(void)
{
	for (int i = 0; i < nsparks; i++)
		move_spark(&spark[i]);
}

static void move_bullets(void)
{
	for (int i = 0; i < nbullets; i++)
		move_bullet(&bullet[i]);
}

static void remove_dead_sparks(void)
{
	int i = 0;

	while (i < nsparks) {
		if (!spark[i].life)
			remove_spark(i);
		else
			i++;
	}
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

static void draw_spark(struct spark *s)
{
	FbColor(YELLOW);
	FbPoint(s->x / 256, s->y / 256);
}

static void draw_bullet(struct bullet *b)
{
	FbColor(WHITE);
	FbLine(b->x / 256, b->y / 256, b->lx / 256, b->ly / 256);
}

static void draw_sparks(void)
{
	for (int i = 0; i < nsparks; i++)
		draw_spark(&spark[i]);
}

static void draw_bullets(void)
{
	for (int i = 0; i < nbullets; i++)
		draw_bullet(&bullet[i]);
}

static void turn_tank(struct tank *t, int amount)
{
	if (t->alive <= 0) /* Sorry, can't turn while dead */
		return;

	t->angle += amount;
	if (t->angle < 0)
		t->angle += 128;
	if (t->angle > 127)
		t->angle -= 128;
}

static void fire_gun(struct tank *t)
{
	int vx, vy;

	if (t->alive <= 0) /* Sorry, can't shoot while dead */
		return;

	vx = -BULLET_SPEED * sine(t->angle) / 256;
	vy = -BULLET_SPEED * cosine(t->angle) / 256;
	add_bullet(t->x, t->y, vx, vy, t - tank); 
}

static void adjust_tank_speed(struct tank *t, int amount)
{
	if (t->alive <= 0) /* Sorry, not if you're dead. */
		return;

	t->speed += amount;
	if (t->speed < MIN_TANK_SPEED)
		t->speed = MIN_TANK_SPEED;
	else if (t->speed > MAX_TANK_SPEED)
		t->speed = MAX_TANK_SPEED;
}

static void check_buttons(void)
{
	int down_latches = button_down_latches();
	int r0 = button_get_rotation(0);
	int r1 = button_get_rotation(1);

	if (r0)
		turn_tank(&tank[0], -2 * r0);
	if (r1)
		turn_tank(&tank[1], -2 * r1);
	/* TODO: make tank turnable using only d-pad */

#if BADGE_HAS_ROTARY_SWITCHES
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		fire_gun(&tank[0]);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches)) {
		fire_gun(&tank[1]);
	} else 
#else
#warning "tank-vs-tank.c needs maintenance related to ROTARY SWITCHES"
#endif
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		adjust_tank_speed(&tank[1], -TANK_SPEED_INCR);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		adjust_tank_speed(&tank[1], TANK_SPEED_INCR);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		adjust_tank_speed(&tank[0], TANK_SPEED_INCR);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		adjust_tank_speed(&tank[0], -TANK_SPEED_INCR);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		tank_vs_tank_state = TANK_VS_TANK_CONFIRM_EXIT;
	}
}

static void respawn_tank(struct tank *t)
{
	int x, y, dx, dy, d2;
	struct tank *enemy_tank = &tank[!(t - tank)];

retry:
	/* Choose a random location */
	x = random_num(LCD_XSIZE);
	y = random_num(LCD_YSIZE);

	/* Compute distance from location to enemy tank */
	dx = x - (enemy_tank->x / 256);
	dy = y - (enemy_tank->y / 256);
	d2 = dx * dx + dy * dy;

	if (d2 < 60 * 60) /* Too close to enemy tank? Try again */
		goto retry;

	/* Touching any obstacles? Try again. */
	for (int i = 0; i < nobstacles; i++)
		if (point_obstacle_collision(x, y, &obstacle[i]))
			goto retry;

	/* Location is good. Move the tank there. */
	t->x = x * 256;
	t->y = y * 256;
	t->speed = 0;
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

	if (nx < 0 || nx >= 256 * LCD_XSIZE || ny < 0 || ny >= 256 * LCD_YSIZE)
		goto stop_tank;

	if (tank_obstacle_collision(nx, ny))
		goto stop_tank;

	if (tank_tank_collision(t, nx, ny))
		goto stop_tank;

	t->x = nx;
	t->y = ny;
	return;

stop_tank:
	t->speed = 0;
}

static void move_objects(void)
{
	move_tank(&tank[0]);
	move_tank(&tank[1]);
	move_bullets();
	remove_dead_bullets();
	move_sparks();
	remove_dead_sparks();
}

static void draw_obstacle(struct obstacle *o)
{
	FbMove(o->x, o->y);
	FbRectangle(o->w, o->h);
}

static void draw_obstacles(void)
{
	FbColor(WHITE);
	for (int i = 0; i < nobstacles; i++)
		draw_obstacle(&obstacle[i]);
}

static void draw_objects(void)
{
	draw_tank(&tank[0]);
	draw_tank(&tank[1]);
	draw_bullets();
	draw_obstacles();
	draw_sparks();
	FbSwapBuffers();
}

static void draw_score(void)
{
	char score[10];
	int x;

	FbColor(WHITE);
	snprintf(score, sizeof(score), "%d", tank[1].score);
	FbMove(0, 0);
	FbWriteString(score);
	snprintf(score, sizeof(score), "%d", tank[0].score);
	x = LCD_XSIZE - strlen(score) * 9;
	FbMove(x, 0);
	FbWriteString(score);
}

static void tank_vs_tank_run(void)
{
	check_buttons();
	move_objects();
	draw_objects();
	draw_score();
}

static void tank_vs_tank_exit(void)
{
	tank_vs_tank_state = TANK_VS_TANK_INIT; /* So that when we start again, we do not immediately exit */
	pop_app();
}

static void tank_vs_tank_confirm_exit(void)
{
	dynmenu_draw(&quit_menu);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	int r0 = button_get_rotation(0);

	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		dynmenu_change_current_selection(&quit_menu, 1);
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		dynmenu_change_current_selection(&quit_menu, -1);
	}
	if (r0 > 0)
		dynmenu_change_current_selection(&quit_menu, 1);
	if (r0 < 0)
		dynmenu_change_current_selection(&quit_menu, -1);
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)
#if BADGE_HAS_ROTARY_SWITCHES
		|| BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)
#endif
		) {
	        tank_vs_tank_state = quit_menu.item[quit_menu.current_item].next_state;
	}
}

void tank_vs_tank_cb(__attribute__((unused)) struct badge_app *app)
{
	switch (tank_vs_tank_state) {
	case TANK_VS_TANK_INIT:
		tank_vs_tank_init();
		break;
	case TANK_VS_TANK_INTRO:
		tank_vs_tank_intro();
		break;
	case TANK_VS_TANK_RUN:
		tank_vs_tank_run();
		break;
	case TANK_VS_TANK_CONFIRM_EXIT:
		tank_vs_tank_confirm_exit();
		break;
	case TANK_VS_TANK_EXIT:
		tank_vs_tank_exit();
		break;
	default:
		break;
	}
}

