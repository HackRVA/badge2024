#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "trig.h"
#include "random.h"
#include "string.h"

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
	unsigned char current_room;
} player;

#define CASTLE_FLOORS 5
#define CASTLE_ROWS 3
#define CASTLE_COLS 4
#define NUM_ROOMS (CASTLE_FLOORS * CASTLE_ROWS * CASTLE_COLS)
#define NUM_DESKS 30
#define GULAG_MAX_OBJS_PER_ROOM 5
#define GULAG_MAXOBJS (CASTLE_FLOORS * CASTLE_COLS * CASTLE_ROWS * GULAG_MAX_OBJS_PER_ROOM)

struct room_spec {
	/* Each room has up to 4 doors (top, bottom, left, and right).
	 * They are encoded into two bits.  Because the rooms are in a
	 * grid, each room only stores the presence/absence of the left
	 * and top doors, and determines the presense/absence of the right
	 * and bottom doors by examining the neighboring room to the right
	 * and below for left/top doors, respectively.
	 */
	int doors : 2;
#define HAS_LEFT_DOOR (1 << 0)
#define HAS_TOP_DOOR (1 << 1)
	int16_t obj[GULAG_MAX_OBJS_PER_ROOM]; /* indices into go[], below */
	char nobjs;
};

static struct castle {
	struct room_spec room[NUM_ROOMS];
	int exit_room;
	int start_room;
	int stairs_down[CASTLE_FLOORS];
	int stairs_up[CASTLE_FLOORS];
} castle;

static enum gulag_state_t gulag_state = GULAG_INIT;
static int screen_changed = 0;
static int intro_offset = 0;
static int flag_offset = 0;

struct gulag_stairs_data {
	uint16_t unused;
};

struct gulag_desk_data {
	uint16_t keys:1;
	uint16_t war_plans:1;
	uint16_t bullets:3;
	uint16_t vodka:1;
	uint16_t locked:1;
};

struct gulag_soldier_data {
	uint16_t health:3;
	uint16_t bullets:3;
	uint16_t spetsnaz:1;
	uint16_t grenades:2;
	uint16_t keys:1;
	uint16_t weapon:2;
};

struct gulag_chest_data {
	uint16_t bullets:3;
	uint16_t grenades:3;
	uint16_t weapons:2;
	uint16_t war_plans:1;
	uint16_t explosives:1;
};

union gulag_type_specific_data {
	struct gulag_stairs_data stairs;
	struct gulag_desk_data desk;
	struct gulag_soldier_data soldier;
	struct gulag_chest_data chest;
};

struct gulag_object {
	uint16_t room;
	uint16_t x, y;
	unsigned char type;
#define TYPE_STAIRS_UP 0
#define TYPE_STAIRS_DOWN 1
#define TYPE_DESK 2
#define TYPE_SOLDIER 3
#define TYPE_CHEST 4
	union gulag_type_specific_data tsd;
} go[GULAG_MAXOBJS];
int gulag_nobjs = 0;

typedef void (*gulag_object_drawing_function)(struct gulag_object *o);

static void draw_stairs_up(struct gulag_object *o);
static void draw_stairs_down(struct gulag_object *o);
static void draw_desk(struct gulag_object *o);
static void draw_soldier(struct gulag_object *o);
static void draw_chest(struct gulag_object *o);

struct gulag_object_typical_data {
	char w, h; /* width, height of bounding box */
	gulag_object_drawing_function draw;
} objconst[] = {
	{ 32, 32, draw_stairs_up, },
	{ 32, 32, draw_stairs_down, },
	{ 16, 8, draw_desk, },
	{ 8, 16, draw_soldier, },
	{ 16, 8, draw_chest, },
};

static void draw_stairs_up(struct gulag_object *o)
{
	int x, y;

	x = o->x >> 8;
	y = o->y >> 8;

	FbColor(WHITE);
	FbMove(x, y);
	FbRectangle(32, 12);

	for (int i = 0; i < 5; i++) {
		FbMove(x + 15, y + 12 + i * 4);
		FbRectangle(16 - i, 5);
	}
	FbLine(x, y + 12, x + 8, y + 24);
	FbHorizontalLine(x + 8, y + 24, x + 15, y + 24);
}

static void draw_stairs_down(struct gulag_object *o)
{
	int x, y;

	x = o->x >> 8;
	y = o->y >> 8;

	FbColor(WHITE);
	FbMove(x, y);
	FbRectangle(32, 32); /* outer box */
	FbMove(x + 4, y + 4);

	FbRectangle(32 - 8, 12); /* landing */
	FbLine(x, y, x + 4, y + 4);
	FbLine(x + 31, y, x + 31 - 4, y + 4);

	/* right hand steps */
	for (int i = 0; i < 4; i++) {
		FbMove(x + 16, y + 15 + i * 4);
		FbRectangle(12 + i, 5);
	}

	/* left hand steps */
	for (int i = 0; i < 4; i++) {
		FbMove(x + 4 + i, y + 16 + i * 2);
		FbRectangle(16 - 4 - i + 1, 3);
	}

	FbVerticalLine(x + 7, y + 16 + 3 * 2, x + 7, y + 28);
	FbHorizontalLine(x + 7, y + 28, x + 16, y + 28);
	FbLine(x, y + 31, x + 7, y + 28);
}

static void draw_desk(struct gulag_object *o)
{
	int x, y;

	x = o->x >> 8;
	y = o->y >> 8;

	FbHorizontalLine(x, y + 2, x + 16, y + 2);
	FbHorizontalLine(x + 4, y, x + 12, y);
	FbLine(x + 4, y, x, y + 2);
	FbLine(x + 12, y, x + 16, y + 2);
	FbVerticalLine(x + 2, y + 2, x + 2, y + 8);
	FbVerticalLine(x + 14, y + 2, x + 14, y + 8);
	FbVerticalLine(x + 5, y + 2, x + 5, y + 8);
	FbVerticalLine(x + 11, y + 2, x + 11, y + 8);
	FbHorizontalLine(x + 2, y + 8, x + 5, y + 8);
	FbHorizontalLine(x + 11, y + 8, x + 14, y + 8);
}

static void draw_soldier(__attribute__((unused)) struct gulag_object *o)
{
}

static void draw_chest(__attribute__((unused)) struct gulag_object *o)
{
}

/* return a random int between 0 and n - 1 */
static int random_num(int n)
{
	int x;
	random_insecure_bytes((uint8_t *) &x, sizeof(x));
	if (x < 0)
		x = -x;
	return x % n;
}

static int room_no(int floor, int col, int row)
{
	return floor * (CASTLE_ROWS * CASTLE_COLS) + col * CASTLE_ROWS + row;
}

static int get_room_floor(int room)
{
	return room / (CASTLE_COLS * CASTLE_ROWS);
}

static int get_room_col(int room)
{
	int r = room % (CASTLE_COLS * CASTLE_ROWS);
	return r / CASTLE_ROWS;
}

static int get_room_row(int room)
{
	int r = room % (CASTLE_COLS * CASTLE_ROWS);
	return r % CASTLE_ROWS;
}

static int has_left_door(struct castle *c, int room)
{
	return c->room[room].doors & HAS_LEFT_DOOR;
}

static int has_top_door(struct castle *c, int room)
{
	return c->room[room].doors & HAS_TOP_DOOR;
}

static int has_bottom_door(struct castle *c, int room)
{
	int row;

	row = get_room_row(room);
	if (row == CASTLE_ROWS - 1)
		return (room == c->exit_room);
	return (c->room[room + 1].doors & HAS_TOP_DOOR);
}

static int has_right_door(struct castle *c, int room)
{
	int col;

	col = get_room_col(room);
	if (col == CASTLE_COLS - 1)
		return room == castle.exit_room;
	return (c->room[room + CASTLE_ROWS].doors & HAS_LEFT_DOOR);
}

static void add_start_room(struct castle *c)
{
	int row, col;

	row = random_num(CASTLE_ROWS);
	col = random_num(CASTLE_COLS);
	c->start_room = room_no(CASTLE_FLOORS - 1, col, row);
}

static void add_exit_room(struct castle *c)
{
	(void) c;
}

static void add_staircase(struct castle *c, int floor, int col, int row, int stair_direction)
{
	int i = gulag_nobjs;
	int room = room_no(floor, col, row);
	go[i].type = stair_direction;
	go[i].room = room;
	go[i].x = (127 - objconst[stair_direction].w) << 8;
	go[i].y = 0 << 8;
	gulag_nobjs++;

	int n = c->room[room].nobjs;
	c->room[room].obj[n] = i;
	c->room[room].nobjs++;
}

static void add_stairs(struct castle *c)
{
	int row, col;

	for (int i = CASTLE_FLOORS - 1; i > 0; i--) {
		row = random_num(CASTLE_ROWS);
		col = random_num(CASTLE_COLS);
		add_staircase(c, i, col, row, TYPE_STAIRS_DOWN);
		add_staircase(c, i - 1, col, row, TYPE_STAIRS_UP);
	}
}

static int room_contains_obj_of_type(struct castle *c, int room, int type)
{
	for (int i = 0; i < c->room[room].nobjs; i++) {
		int n = c->room[room].obj[i];
		if (go[n].type == type)
			return 1;
	}
	return 0;
}

static int add_object_to_room(struct castle *c, int room, int object_type, int x, int y)
{
	int n = gulag_nobjs;
	if (n >= GULAG_MAXOBJS)
		return -1;
	if (c->room[room].nobjs >= GULAG_MAX_OBJS_PER_ROOM)
		return -1;
	go[n].type = object_type;
	go[n].room = room;
	/* TODO: move things around so nothing collides */
	go[n].x = x;
	go[n].y = y;
	gulag_nobjs++;
	int i = c->room[room].nobjs;
	c->room[room].obj[i] = n;
	c->room[room].nobjs++;
	return n;
}

static void add_desk(struct castle *c)
{
	int floor, col, row, room;

	do {
		floor = random_num(CASTLE_FLOORS);
		row = random_num(CASTLE_ROWS);
		col = random_num(CASTLE_COLS);
		room = room_no(floor, col, row);
	} while (room_contains_obj_of_type(c, room, TYPE_DESK));

	int x = random_num(127 - 40) + 8;
	int y = random_num(127 - 16 - 16);

	int n = add_object_to_room(c, room, TYPE_DESK, x << 8, y << 8);
	if (n < 0)
		return;
	go[n].tsd.desk.bullets = random_num(8);
	go[n].tsd.desk.keys = (random_num(100) < 20);
	go[n].tsd.desk.war_plans = 0;
	go[n].tsd.desk.locked = (random_num(100) < 50);
}

static void add_desks(struct castle *c)
{
	for (int i = 0; i < NUM_DESKS; i++)
		add_desk(c);
}

static void add_doors(struct castle *c, int floor, int start_col, int end_col, int start_row, int end_row)
{
	int rows = end_row - start_row + 1;
	int cols = end_col - start_col + 1;
	int r1, r2, r3, r4, c1, c2, c3, c4, room;

	if (rows == 1 && cols == 1)
		return;
	if (rows == 1 && cols == 2) {
		room = room_no(floor, end_col, start_row);
		c->room[room].doors |= HAS_LEFT_DOOR;
		return;
	}
	if (rows == 2 && cols == 1) {
		room = room_no(floor, start_col, end_row);
		c->room[room].doors |= HAS_TOP_DOOR;
		return;
	}

	if (cols >= rows) {
		/* more columns than rows, choose a column randomly and add a door */
		c3 = random_num((cols - 1)) + start_col + 1;
#if 0
		random_insecure_bytes((uint8_t *) &c3, sizeof(c3));
		if (c3 < 0)
			c3 = -c3;
		c3 = c3 % (cols - 1) + start_col + 1;
#endif
		r3 = rows / 2 + start_row;
		room = room_no(floor, c3, r3);
		c->room[room].doors |= HAS_LEFT_DOOR;

		/* Subdivide */
		c1 = start_col;
		c2 = c3 - 1;
		if (c2 < c1)
			c2 = c1;
		c4 = end_col;
		add_doors(c, floor, c1, c2, start_row, end_row);
		add_doors(c, floor, c3, c4, start_row, end_row);
		return;
	}

	/* more rows than columns, choose a row randomly and add a door */
	r3 = random_num(rows - 1) + start_row + 1;
#if 0
	random_insecure_bytes((uint8_t *) &r3, sizeof(r3));
	if (r3 < 0)
		r3 = -r3;
	r3 = r3 % (rows - 1) + start_row + 1;
#endif
	c3 = cols / 2 + start_col;
	room = room_no(floor, c3, r3);
	c->room[room].doors |= HAS_TOP_DOOR;

	/* Subdivide */
	r1 = start_row;
	r2 = r3 - 1;
	if (r2 < r1)
		r2 = r1;
	r4 = end_row;
	add_doors(c, floor, start_col, end_col, r1, r2);
	add_doors(c, floor, start_col, end_col, r3, r4);
}

static void init_castle(struct castle *c)
{
	int i;

	/* Start with no doors. */
	for (i = 0; i < NUM_ROOMS; i++) {
		c->room[i].doors = 0;
		memset(c->room[i].obj, 0, sizeof(c->room[i].obj));
		c->room[i].nobjs = 0;
	}
	for (i = 0; i < CASTLE_FLOORS; i++)
		add_doors(&castle, i, 0, CASTLE_COLS - 1, 0, CASTLE_ROWS - 1);
	add_start_room(c);
	add_exit_room(c);
	add_stairs(&castle);
	add_desks(&castle);
}

#if TARGET_SIMULATOR
static void print_castle_row(struct castle *c, struct player *p, int f, int r)
{
	int room;
	char player_marker;

	for (int i = 0; i < CASTLE_COLS; i++) {
		room = room_no(f, i, r);
		if (has_top_door(c, room))
			printf("+-  -");
		else
			printf("+----");
	}
	printf("+\n");

	for (int i = 0; i < CASTLE_COLS; i++)
		printf("|%4d", room_no(f, i, r));
	printf("|\n");

	for (int i = 0; i < CASTLE_COLS; i++) {
		room = room_no(f, i, r);
		if (room == p->room)
			player_marker = '*';
		else
			player_marker = ' ';
		if (has_left_door(c, room))
			printf("  %c  ", player_marker);
		else
			printf("| %c  ", player_marker);
	}
	printf("|\n");

	for (int i = 0; i < CASTLE_COLS; i++)
		printf("|    ");
	printf("|\n");
}

static void print_castle_floor(struct castle *c, struct player *p, int f)
{
	printf("\n\nFLOOR %d\n", f);
	for (int i = 0; i < CASTLE_ROWS; i++)
		print_castle_row(c, p, f, i);
	for (int i = 0; i < CASTLE_COLS; i++)
		printf("+----");
	printf("+\n");
}
#endif

#if TARGET_SIMULATOR
static void print_castle(struct castle *c, struct player *p)
{
	for (int i = 0; i < CASTLE_FLOORS; i++)
		print_castle_floor(c, p, i);
}
#else
#define print_castle(c, p)
#endif

static void init_player(struct player *p, int start_room)
{
	p->room = start_room;
	p->x = 20 << 8;
	p->y = 20 << 8;
	p->oldx = p->x;
	p->oldy = p->y;
	p->angle = 0;
	p->oldangle = 0;
}

static void gulag_init(void)
{
	gulag_nobjs = 0;
	memset(go, 0, sizeof(go));
	init_castle(&castle);
	init_player(&player, castle.start_room);
	print_castle(&castle, &player);
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
		"IN THE",
		"MUNITIONS",
		"DEPOT AND",
		"STEAL THE",
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
	for (int i = first_line; i < first_line + 14; i++) {
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
	FbFilledRectangle(128, 10);
	FbColor(BLACK);
	FbMove(0, 140);
	FbFilledRectangle(128, 10);
	intro_offset++;
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		gulag_state = GULAG_RUN;
}

static void gulag_flag(void)
{
	if (flag_offset == 80) {
		flag_offset = 0;
		gulag_state = GULAG_RUN;
		return;
	}
	FbMove(0, 0);
	FbColor(BLUE);
	FbFilledRectangle(128, flag_offset);
	FbMove(0, 160 - flag_offset);
	FbColor(YELLOW);
	FbFilledRectangle(128, flag_offset);
	FbSwapBuffers();
	flag_offset++;

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		gulag_state = GULAG_RUN;
}

static void player_wins(void)
{
#if TARGET_SIMULATOR
	printf("You won!\n");
	exit(0);
#endif
}

static void maybe_traverse_stairs(struct castle *c, struct player *p, struct gulag_object *stairs)
{
	int px, py, sx, sy, floor, row, col;

	(void) c; /* shut the compiler up about unused param */

	px = p->x >> 8;
	py = p->y >> 8;
	sx = (stairs->x >> 8) + 16;
	sy = (stairs->y >> 8) + 16;

	/* No collision with stairs... */
	if (abs(px - sx) > 12 || abs(py - sy) > 12)
		return;

	/* Traverse the stairs */
	floor = get_room_floor(p->room);
	col = get_room_col(p->room);
	row = get_room_row(p->room);

	if (stairs->type == TYPE_STAIRS_DOWN)
		floor--;
	else
		floor++;
#if TARGET_SIMULATOR
	print_castle_floor(c, p, floor);
#endif
	p->room = room_no(floor, col, row);
	p->angle = 32; /* down */
	p->x = stairs->x + (24 << 8);
	p->y = stairs->y + ((32 + 8) << 8);
	FbClear();
	screen_changed = 1;
}

static void check_object_collisions(struct castle *c, struct player *p)
{
	int room = p->room;
	for (int i = 0; i < c->room[room].nobjs; i++) {
		int index = c->room[room].obj[i];
		struct gulag_object *o = &go[index];
		int t = o->type;
		switch (t) {
		case TYPE_STAIRS_DOWN:
			/* Fall through */
		case TYPE_STAIRS_UP:
			maybe_traverse_stairs(c, p, o);
			break;
		case TYPE_DESK:
			break;
		case TYPE_SOLDIER:
			break;
		case TYPE_CHEST:
			break;
		}
	}
}

static void check_doors(struct castle *c, struct player *p)
{
	int room = p->room;
	int x, y, f, row, col;

	x = p->x >> 8;
	y = p->y >> 8;
	f = get_room_floor(room);
	row = get_room_row(room);
	col = get_room_col(room);
	if (has_top_door(c, room)) {
		if (y <= 10 && x > 60 && x < 68) {
			row--;
			if (row < 0)
				player_wins();
			p->room = room_no(f, col, row);
			FbClear();
			p->y = (127 - 16 - 12) << 8;
#if TARGET_SIMULATOR
			print_castle_floor(c, p, f);
#endif
			return;
		}
	}
	if (has_bottom_door(c, room)) {
		if (y >= 127 - 16 - 10 && x > 60 && x < 68) {
			row++;
			if (row >= CASTLE_ROWS)
				player_wins();
			p->room = room_no(f, col, row);
			FbClear();
			p->y = 18 << 8;
#if TARGET_SIMULATOR
			print_castle_floor(c, p, f);
#endif
			return;
		}
	}
	if (has_left_door(c, room)) {
		if (x < 6 && y > 54 && y < 74) {
			col--;
			if (col < 0)
				player_wins();
			p->room = room_no(f, col, row);
			FbClear();
			p->x = 117 << 8;
#if TARGET_SIMULATOR
			print_castle_floor(c, p, f);
#endif
			return;
		}
	}
	if (has_right_door(c, room)) {
		if (x >121 && y > 54 && y < 74) {
			col++;
			if (col >= CASTLE_COLS)
				player_wins();
			p->room = room_no(f, col, row);
			FbClear();
			p->x = 10 << 8;
#if TARGET_SIMULATOR
			print_castle_floor(c, p, f);
#endif
			return;
		}
	}
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
		check_doors(&castle, &player);
		check_object_collisions(&castle, &player);
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

static void draw_room_objs(struct castle *c, int room)
{

	for (int i = 0; i < c->room[room].nobjs; i++) {
		int n = c->room[room].obj[i];
		struct gulag_object *o = &go[n];
		objconst[o->type].draw(o);
	}
}

static void draw_room(struct castle *c, int room)
{
	if (has_top_door(c, room)) {
		FbHorizontalLine(0, 0, 54, 0);
		FbHorizontalLine(74, 0, 127, 0);
	} else {
		FbHorizontalLine(0, 0, 127, 0);
	}
	if (has_left_door(c, room)) {
		FbVerticalLine(0, 0, 0, 54);
		FbVerticalLine(0, 74, 0, 127 - 16);
	} else {
		FbVerticalLine(0, 0, 0, 127 - 16);
	}
	if (has_bottom_door(c, room)) {
		FbHorizontalLine(0, 127 - 16, 54, 127 - 16);
		FbHorizontalLine(74, 127 - 16, 127, 127 - 16);
	} else {
		FbHorizontalLine(0, 127 - 16, 127, 127 - 16);
	}
	if (has_right_door(c, room)) {
		FbVerticalLine(127, 0, 127, 54);
		FbVerticalLine(127, 74, 127, 127 - 16);
	} else {
		FbVerticalLine(127, 0, 127, 127 - 16);
	}
	draw_room_objs(c, room);
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	draw_room(&castle, player.room);
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

