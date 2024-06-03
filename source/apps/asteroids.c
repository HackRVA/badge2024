#include <stdio.h>
#include <assert.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "trig.h"
#include "fxp_sqrt.h"
#include "xorshift.h"
#include "random.h"
#include "dynmenu.h"
#include <string.h>

#if TARGET_PICO
#define printf(...)
#endif

#define MAXBULLETS 10
#define MAXSPARKS 100
#define MAXASTEROIDS 100
#define NUM_ASTEROID_FORMS 10
#define NUM_INITIAL_ASTEROIDS 4
#define FIRE_COOLDOWN 5
static int fire_cooldown = 0;
static const short player_rotation_speed = 3;
static const short player_thrust_amount = 64;
static const int max_speed = (10 << 8);
static const int max_speed_squared = (((max_speed >> 8) * (max_speed >> 8)) << 8);
static const int bullet_speed = (4 << 8);
static const int bullet_life = 30;
static const int min_asteroid_radius = 32;
static int lives;
static int score;
static int player_dead_counter;
static int game_over_counter;

static struct dynmenu quitmenu;
static struct dynmenu_item quitmenu_item[2];

struct pos_vel {
	int x, y, vx, vy;
};

static struct ship {
	struct pos_vel p;
	int angle;
} player;

static struct bullet {
	struct pos_vel p;
	signed char life;
} bullet[MAXBULLETS] = { 0 };
static int nbullets = 0;

static struct spark {
	struct pos_vel p;
	signed char life;
} spark[MAXSPARKS];
static int nsparks = 0;

static struct asteroid_form {
	int x[10];
	int y[10];
} asteroid_form[NUM_ASTEROID_FORMS];

static struct asteroid {
	struct pos_vel p;
	int radius;
	int form;
} asteroid[MAXASTEROIDS] = { 0 };
static int nasteroids;

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

static void generate_asteroid_form(struct asteroid_form *af)
{
	for (int i = 0; i < 10; i++) {
		int a = i * 12; /* 1 / 10th of a circle, roughly */
		int r = (10 << 8) + random_num(5 << 8);
		af->x[i] = (cosine(a) * r) >> 8;
		af->y[i] = (sine(a) * r) >> 8;
	}
}

static void generate_asteroid_forms(void)
{
	for (int i = 0; i < NUM_ASTEROID_FORMS; i++)
		generate_asteroid_form(&asteroid_form[i]);
}

static void add_bullet(int x, int y, int vx, int vy, int life)
{
	if (nbullets >= MAXBULLETS)
		return;
	bullet[nbullets].p.x = x;
	bullet[nbullets].p.y = y;
	bullet[nbullets].p.vx = vx;
	bullet[nbullets].p.vy = vy;
	bullet[nbullets].life = life;
	nbullets++;
}

/* Program states.  Initial state is ASTEROIDS_INIT */
enum asteroids_state_t {
	ASTEROIDS_INIT,
	ASTEROIDS_RUN,
	ASTEROIDS_MAYBE_EXIT,
	ASTEROIDS_EXIT,
};

static enum asteroids_state_t asteroids_state = ASTEROIDS_INIT;
static int screen_changed = 0;

static void init_player(struct ship *player)
{
	player->p.x = (LCD_XSIZE / 2) << 8;
	player->p.y = (LCD_YSIZE / 2) << 8;
	player->p.vx = 0;
	player->p.vy = 0;
	player->angle = 0;
}

static void add_asteroid(int x, int y, int vx, int vy, int radius)
{
	if (nasteroids >= MAXASTEROIDS)
		return;
	struct asteroid *a = &asteroid[nasteroids];
	a->p.x = x;
	a->p.y = y;
	a->p.vx = vx;
	a->p.vy = vy;
	a->form = nasteroids % NUM_ASTEROID_FORMS;
	a->radius = radius;
	nasteroids++;
}

static void remove_asteroid(int n)
{
	if (n != nasteroids - 1)
		asteroid[n] = asteroid[nasteroids - 1];
	nasteroids--;
}

static void add_initial_asteroids(void)
{
	int x, y, vx, vy;
	nasteroids = 0;
	for (int i = 0; i < NUM_INITIAL_ASTEROIDS; i++) {
		x = random_num(LCD_XSIZE);
		y = random_num(LCD_YSIZE);
		vx = random_num(1 << 8) - (1 << 7);
		vy = random_num(1 << 8) - (1 << 7);
		add_asteroid(x, y, vx, vy, 1 << 8);
	}
}

static void asteroids_init(void)
{
	FbInit();
	FbClear();
	asteroids_state = ASTEROIDS_RUN;
	screen_changed = 1;
	generate_asteroid_forms();
	add_initial_asteroids();
	init_player(&player);
	lives = 3;
	score = 0;
	player_dead_counter = 0;
	game_over_counter = 0;
	nbullets = 0;
	nsparks = 0;
	dynmenu_init(&quitmenu, quitmenu_item, 2);
	dynmenu_clear(&quitmenu);
	strcpy(quitmenu.title, "Quit?");
	dynmenu_add_item(&quitmenu, "No", ASTEROIDS_RUN, 0);
	dynmenu_add_item(&quitmenu, "Yes", ASTEROIDS_EXIT, 0);
}

static void turn(struct ship *player, int angle)
{
	player->angle += angle;
	if (player->angle < 0)
		player->angle += 128;
	if (player->angle > 127)
		player->angle -= 128;
}

static void add_spark(int x, int y, int vx, int vy, int life)
{
	if (nsparks >= MAXSPARKS)
		return;
	struct spark *s = &spark[nsparks];
	s->p.x = x;
	s->p.y = y;
	s->p.vx = vx;
	s->p.vy = vy;
	s->life = life;
	nsparks++;
}

static void thrust(struct ship *player, int thrust_amount)
{
	int dvx, dvy;
	int speed_squared;

	dvx = (cosine(player->angle) * thrust_amount) >> 8;
	dvy = (sine(player->angle) * thrust_amount) >> 8;

	player->p.vx += dvx;
	player->p.vy += dvy;

	speed_squared = (player->p.vx * player->p.vx) >> 8;
	speed_squared += ((player->p.vy * player->p.vy) >> 8);

	if (speed_squared > max_speed_squared) {
		int speed = fxp_sqrt(speed_squared);
		player->p.vx = max_speed * player->p.vx / speed;
		player->p.vy = max_speed * player->p.vy / speed;
	}

	for (int i = 0; i < 10; i++) {
		int svx, svy;

		svx = player->p.vx - 4 * dvx + random_num(64) - 32;
		svy = player->p.vy - 4 * dvy + random_num(64) - 32;
		add_spark(player->p.x, player->p.y, svx, svy, 15);
	}
}

static void fire(struct ship *player)
{
	int vx, vy;

	vx = (cosine(player->angle) * bullet_speed) >> 8;
	vy = (sine(player->angle) * bullet_speed) >> 8;
	vx += player->p.vx;
	vy += player->p.vy;
	add_bullet(player->p.x , player->p.y, vx, vy, bullet_life);
	fire_cooldown = FIRE_COOLDOWN;
}

static void add_random_spark(int x, int y, int v)
{
	int angle;

	if (nsparks >= MAXSPARKS)
		return;
	struct spark *s = &spark[nsparks];
	angle = random_num(128);
	s->p.x = x;
	s->p.y = y;
	int vel = (v / 2) + random_num(v / 2);
	s->p.vx = (cosine(angle) * vel) >> 8;
	s->p.vy = (sine(angle) * vel) >> 8;
	s->life = 15 + random_num(15);
	nsparks++;
}

static void add_sparks(int x, int y, int v, int n)
{
	for (int i = 0; i < n; i++)
		add_random_spark(x, y, v);
}

static void check_buttons(void)
{
	static int first_time = 1;
	int r0 = button_get_rotation(0);
	int r1 = button_get_rotation(1);
	int down_latches = button_down_latches();
	if (game_over_counter || player_dead_counter)
		return;
	if (
#if BADGE_HAS_ROTARY_SWITCHES
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
#endif
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		asteroids_state = ASTEROIDS_MAYBE_EXIT;
	}
	if (r0)
		turn(&player, 2 * r0);
	if (r1)
		turn(&player, 2 * r1);
	if (button_poll(BADGE_BUTTON_LEFT))
		turn(&player, -player_rotation_speed);
	if (button_poll(BADGE_BUTTON_RIGHT))
		turn(&player, player_rotation_speed);
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
		thrust(&player, player_thrust_amount);
	if (button_poll(BADGE_BUTTON_B) && fire_cooldown == 0)
		fire(&player);

	if (fire_cooldown)
		fire_cooldown--;

	if (first_time)
		first_time = 0;

}

static int onscreen(int x, int y)
{
	return (x >= 0 && x < LCD_XSIZE) && (y >= 0 && y < LCD_YSIZE);
}

static void draw_player(struct ship *player)
{
	int x1, y1, x2, y2, x3, y3;
	int dx, dy, a;
	const int ship_size = 5;
	int p1_onscreen, p2_onscreen, p3_onscreen;

	if (player_dead_counter)
		return;

	a = player->angle;
	dx = cosine(a) * ship_size;
	dy = sine(a) * ship_size;
	x1 = (player->p.x + dx) >> 8;
	y1 = (player->p.y + dy) >> 8;

	a = (player->angle + 48); /* +135 degrees */
	while (a > 127)
		a = a - 128;
	dx = cosine(a) * ship_size;
	dy = sine(a) * ship_size;
	x2 = (player->p.x + dx) >> 8;
	y2 = (player->p.y + dy) >> 8;

	a = (player->angle - 48); /* -135 degrees */
	while (a < 0)
		a = a + 128;
	dx = cosine(a) * ship_size;
	dy = sine(a) * ship_size;
	x3 = (player->p.x + dx) >> 8;
	y3 = (player->p.y + dy) >> 8;

	p1_onscreen = onscreen(x1, y1);
	p2_onscreen = onscreen(x2, y2);
	p3_onscreen = onscreen(x3, y3);
	if (p1_onscreen && p2_onscreen)
		FbLine(x1, y1, x2, y2);
	if (p2_onscreen && p3_onscreen)
		FbLine(x2, y2, x3, y3);
	if (p3_onscreen && p1_onscreen)
		FbLine(x3, y3, x1, y1);
}

static void apply_position_delta(struct pos_vel *p)
{
	p->x += p->vx;
	p->y += p->vy;
	if (p->x < 0)
		p->x += (LCD_XSIZE << 8);
	if (p->x >= (LCD_XSIZE << 8))
		p->x -= (LCD_XSIZE << 8);
	if (p->y < 0)
		p->y += (LCD_YSIZE << 8);
	if (p->y >= LCD_YSIZE << 8)
		p->y -= (LCD_YSIZE << 8);
}

static void check_player_asteroid_collision(struct ship *p)
{
	for (int i = 0; i < nasteroids; i++) {
		struct asteroid *a = &asteroid[i];
		int dx = (p->p.x >> 8) - (a->p.x >> 8);
		int dy = (p->p.y >> 8) - (a->p.y >> 8);
		int dist_squared = (dx * dx) + (dy * dy);
		if (dist_squared < (12 * a->radius >> 8) * (12 * a->radius >> 8)) {
			player_dead_counter = 100;
			add_sparks(p->p.x, p->p.y, 3 << 8, 20);
			init_player(p);
			lives--;
		}
	}
}

static void move_player(struct ship *player)
{
	if (player_dead_counter) {
		player_dead_counter--;
		return;
	}
	apply_position_delta(&player->p);
	check_player_asteroid_collision(player);
}

static void check_bullet_asteroid_collision(struct bullet *b)
{
	for (int i = 0; i < nasteroids; i++) {
		struct asteroid *a = &asteroid[i];
		int dx = (b->p.x >> 8) - (a->p.x >> 8);
		int dy = (b->p.y >> 8) - (a->p.y >> 8);
		int dist_squared = (dx * dx) + (dy * dy);
		if (dist_squared < (12 * a->radius >> 8) * (12 * a->radius >> 8)) {
			int r = a->radius / 2;
			if (r >= 256)
				score += 20;
			else if (r >= 128)
				score += 50;
			else if (r >= 64)
				score += 100;
			if (r > min_asteroid_radius) {
				int vx = random_num(1 << 8) - (1 << 7);
				int vy = random_num(1 << 8) - (1 << 7);
				add_asteroid(a->p.x, a->p.y, vx, vy, r);
				vx = random_num(1 << 8) - (1 << 7);
				vy = random_num(1 << 8) - (1 << 7);
				add_asteroid(a->p.x, a->p.y, vx, vy, r);
			}
			remove_asteroid(i);
			add_sparks(b->p.x, b->p.y, 2 << 8, 8);
			b->life = 0;
		}
	}
}

static void move_bullet(struct bullet *b)
{
	apply_position_delta(&b->p);
	b->life--;
	check_bullet_asteroid_collision(b);
}

static void move_spark(struct spark *s) {
	apply_position_delta(&s->p);
	s->life--;
}

static void move_asteroid(struct asteroid *a)
{
	apply_position_delta(&a->p);
}

static void remove_dead_bullets(void)
{
	for (int i = 0; i < nbullets;) {
		if (bullet[i].life <= 0) {
			if (i < nbullets - 1)
				bullet[i] = bullet[nbullets - 1];
			nbullets--;
		} else {
			i++;
		}
	}
}

static void remove_dead_sparks(void)
{
	for (int i = 0; i < nsparks;) {
		if (spark[i].life <= 0) {
			if (i < nsparks - 1)
				spark[i] = spark[nsparks - 1];
			nsparks--;
		} else {
			i++;
		}
	}
}

static void move_bullets(void)
{
	for (int i = 0; i < nbullets; i++)
		move_bullet(&bullet[i]);
	remove_dead_bullets();
}

static void move_sparks(void)
{
	for (int i = 0; i < nsparks; i++)
		move_spark(&spark[i]);
	remove_dead_sparks();
}

static void move_asteroids(void)
{
	for (int i = 0; i < nasteroids; i++)
		move_asteroid(&asteroid[i]);
}

static void draw_bullets(void)
{
	for (int i = 0; i < nbullets; i++) {
		struct bullet *b = &bullet[i];
		if (onscreen(b->p.x / 256, b->p.y / 256))
			FbPoint(b->p.x / 256, b->p.y / 256);
	}
}

static void draw_sparks(void)
{
	FbColor(YELLOW);
	for (int i = 0; i < nsparks; i++) {
		struct spark *s = &spark[i];
		if (onscreen(s->p.x / 256, s->p.y / 256))
			FbPoint(s->p.x / 256, s->p.y / 256);
	}
}

static void draw_asteroid(struct asteroid *a)
{
	int x0, y0, x1, y1, x2, y2;
	int o1, o2;
	struct asteroid_form *f = &asteroid_form[a->form];

	x1 = (a->p.x / 256) + ((f->x[0] * a->radius) / (256 * 256));
	y1 = (a->p.y / 256) + ((f->y[0] * a->radius) / (256 * 256));
	x0 = x1;
	y0 = y1;
	for (int i = 1; i < 10; i++) {
		x2 = (a->p.x / 256) + ((f->x[i] * a->radius) / (256 * 256));
		y2 = (a->p.y / 256) + ((f->y[i] * a->radius) / (256 * 256));
		o1 = onscreen(x1, y1);
		o2 = onscreen(x2, y2);
		if (o1 && o2) {
			FbLine(x1, y1, x2, y2);
		} else if (o1 || o2) {
			FbClippedLine(x1, y1, x2, y2);
		}
		x1 = x2;
		y1 = y2;
	}
	if (onscreen(x0, y0) && onscreen(x1, y1))
		FbLine(x0, y0, x1, y1);
}

static void draw_asteroids(void)
{
	for (int i = 0; i < nasteroids; i++) {
		struct asteroid *a = &asteroid[i];
		draw_asteroid(a);
	}
}

static void draw_score(void)
{
	char scorestr[20];
	FbColor(WHITE);
	FbMove(2, 2);
	snprintf(scorestr, sizeof(scorestr), "%d", score);
	FbWriteString(scorestr);
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	move_player(&player);
	move_bullets();
	move_sparks();
	move_asteroids();
	draw_player(&player);
	draw_asteroids();
	draw_bullets();
	draw_sparks();
	draw_score();
	if (game_over_counter) {
		FbMove(20, LCD_YSIZE / 2);
		FbWriteString("GAME OVER");
	}
	FbSwapBuffers();
	if (nasteroids == 0)
		add_initial_asteroids();
	if (lives == 0 && game_over_counter == 0)
		game_over_counter = 100;
	if (game_over_counter == 1) {
		asteroids_init();
	}
	if (game_over_counter > 0)
		game_over_counter--;
	screen_changed = 1;
}

static void asteroids_run(void)
{
	check_buttons();
	draw_screen();
}

static void asteroids_maybe_exit(void)
{
	if (!dynmenu_let_user_choose(&quitmenu))
		return;

	int choice = dynmenu_get_user_choice(&quitmenu);
	if (choice == DYNMENU_SELECTION_ABORTED)
		asteroids_state = ASTEROIDS_RUN;
	else
		asteroids_state = quitmenu.item[quitmenu.current_item].next_state;
}

static void asteroids_exit(void)
{
	asteroids_state = ASTEROIDS_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void asteroids_cb(__attribute__((unused)) struct badge_app *app)
{
	switch (asteroids_state) {
	case ASTEROIDS_INIT:
		asteroids_init();
		break;
	case ASTEROIDS_RUN:
		asteroids_run();
		break;
	case ASTEROIDS_MAYBE_EXIT:
		asteroids_maybe_exit();
		break;
	case ASTEROIDS_EXIT:
		asteroids_exit();
		break;
	default:
		break;
	}
}

