/*
 * "Goodbye, Gulag!" by Stephen M. Cameron
 *
 * Find the Russian war plans, plant a bomb in the munitions depot, and
 * make your escape from the Russian compound.
 *
 * Inspired by the 1981 Apple ][ game, "Castle Wolfenstein" by Silas Warner.
 *
 */


/*
 * Things remaining to do:
 *
 * Enemy behaviors:
 *   (X) Shooting
 *   ( ) Chasing
 *   ( ) Fleeing
 *   ( ) Surrendering/being searched
 *   ( ) Following room to room (Spetsnaz only)
 *
 * Player behaviors:
 *   (X) Searching chests/desks/bodies for bullets, keys, etc.
 *   (X) Inventory of bullets/grenades/keys/bullet proof vest etc.
 *   ( ) planting bomb
 *   (X) throwing grenades
 *   ( ) knifing?
 *   (X) health damage/dying
 *   (X) healing w/ medicine/food
 *
 * Environment features
 *   ( ) Locked doors
 *   (X) Keys
 *   (X) food and drink (vodka, cabbages, potatoes).
 *   ( ) the munitions depot
 *   (X) the war plans
 *   (X) plastic explosives
 *   (X) remote detonator
 *   (X) Win condition/scene
 *   (X) Lose condition/scene
 *   (X) Safes, and safe-cracking mini game
 *   ( ) Flamethrowers
 *
 * Quality of Life
 *   (X) Difficulty levels (easy/medium/hard/insane)
 *   ( ) Help screen
 *   (X) Quit confirmation screen
 *   ( ) Building map?
 *
 * Optimizations
 *   (X) Every soldier in the game does not need pathfinding memory,
 *       only every soldier *in the current room* needs pathfinding
 *       memory.
 */


#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "trig.h"
#include "random.h"
#include "string.h"
#include "bline.h"
#include "a_star.h"
#include "dynmenu.h"
#include "led_pwm.h"
#include "rtc.h"

static struct dynmenu start_menu;
#define START_MENU_SIZE 5
static struct dynmenu_item start_menu_item[START_MENU_SIZE];
static struct dynmenu quit_menu;
static struct dynmenu_item quit_menu_item[2];

/* Program states.  Initial state is GULAG_INIT */
enum gulag_state_t {
	GULAG_INIT,
	GULAG_INTRO,
	GULAG_SCROLL_TEXT,
	GULAG_FLAG,
	GULAG_START_MENU,
	GULAG_RUN,
	GULAG_SAFECRACKING,
	GULAG_SAFE_CRACKED,
	GULAG_PLAYER_DIED,
	GULAG_MAYBE_EXIT,
	GULAG_PLAYER_WINS,
	GULAG_PRINT_STATS,
	GULAG_VIEW_COMBO,
	GULAG_EXIT,
};

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#ifdef TARGET_PICO
/* printf?  What printf? */
#define printf(...)
#endif

#define CASTLE_FLOORS 5
#define CASTLE_ROWS 3
#define CASTLE_COLS 4
#define NUM_ROOMS (CASTLE_FLOORS * CASTLE_ROWS * CASTLE_COLS)
#define NUM_DESKS 30
#define NUM_CHESTS 30
/* NUM_SOLDIERS is nominally 120, 2 per room, on average */
#define NUM_SOLDIERS (CASTLE_FLOORS * CASTLE_ROWS * CASTLE_COLS * 2)
#define GULAG_MAX_OBJS_PER_ROOM 5
#define GULAG_MAXOBJS (CASTLE_FLOORS * CASTLE_COLS * CASTLE_ROWS * GULAG_MAX_OBJS_PER_ROOM)

struct difficulty_settings {
	short nsoldiers;
	short soldier_speed;
	short soldier_shoot_cooldown;
	short bullet_damage;
	short chest_grenade_chance;
	short soldier_grenade_chance;
	short chest_first_aid_chance;
	short num_desk_safe_combo;
} difficulty[] = {
	{ NUM_SOLDIERS / 2,		128, 30, 1 << 8, 1000,	300, 500, 10, }, /* easy */
	{ (3 * NUM_SOLDIERS) / 2,	200, 25, 2 << 8, 800,	200, 400,  8, }, /* medium */
	{ NUM_SOLDIERS,			256, 20, 3 << 8, 600,	200, 300,  5, }, /* hard */
	{ NUM_SOLDIERS,			300, 15, 4 << 8, 300,	100, 200,  5, }, /* insane */
};
static int difficulty_level;

static const short player_speed = 512;
static const short grenade_speed = 512;
static const short grenade_speed_reduction = 128;
#define GRENADE_FRAGMENTS 40
static int game_timer = 0;
static char safe_combo[CASTLE_FLOORS][20];

enum search_state {
	search_state_idle = 0, /* not searching right now */
#define SEARCH_INIT_TIME 30
#define MIN_SEARCH_DIST2 (15 * 15)
	search_state_searching,
#define SEARCH_TIME (30 * 2)
	search_state_trying_keys,
#define SEARCH_KEY_TIME 20
	search_state_unlocked,
#define SEARCH_UNLOCKED_TIME 30
	search_state_no_dice,
#define SEARCH_NO_DICE_TIME 30
	search_state_locked,
#define SEARCH_LOCKED_TIME 30
	search_state_finding_items,
#define SEARCH_ITEM_TIME 30
	search_state_cooldown,
#define SEARCH_COOLDOWN_TIME 30
};

static struct player {
	short room;
	short x, y; /* 8.8 signed fixed point */
	short oldx, oldy;
	short bullets, grenades;
	short health; /* 8.8 fixed point */
#define MAX_PLAYER_HEALTH (100 << 8)
#define POTATO_HEALTH_BOOST (3 << 8);
#define CABBAGE_HEALTH_BOOST (4 << 8);
#define VODKA_HEALTH_BOOST (5 << 8);
#define FIRST_AID_HEALTH_BOOST (10 << 8);
	unsigned char have_war_plans;
	unsigned char keys;
	unsigned char trying_key;
	int kills;
	unsigned char angle, oldangle; /* 0 - 127, 0 is to the left, 32 is down, 64 is right, 96 is up. */
	unsigned char current_room;
	char anim_frame, prev_frame;
	enum search_state search_state;
	int search_timer;
	int search_item_num;
	int search_object;
	int safecracking_cooldown;
#define SAFECRACK_COOLDOWN_TIME 60
	int current_safe;
	uint16_t documents_collected;
	int has_won;
	uint64_t start_time_ms;
	uint64_t end_time_ms;
	int shots_fired;
	int grenades_thrown;
	unsigned char has_combo[CASTLE_FLOORS];
	unsigned char has_detonator;
	unsigned char has_c4;
} player;

static char player_message[100] = { 0 };
static int display_message = 0;

#define COST_XDIM 32
#define COST_YDIM 32
#define ASTAR_MAXNODES (COST_XDIM * COST_YDIM)
/* A-star cost heuristic, see init_room_cost(), how soldiers avoid walls. */
static unsigned char room_cost[COST_YDIM][COST_XDIM];

/* Workspace for A* algorithm. */
static unsigned char nodeset1[A_STAR_NODESET_SIZE(ASTAR_MAXNODES)];
static unsigned char nodeset2[A_STAR_NODESET_SIZE(ASTAR_MAXNODES)];
static unsigned char scoremap1[A_STAR_SCOREMAP_SIZE(ASTAR_MAXNODES)];
static unsigned char scoremap2[A_STAR_SCOREMAP_SIZE(ASTAR_MAXNODES)];
static unsigned char nodemap[A_STAR_NODEMAP_SIZE(ASTAR_MAXNODES)];
static unsigned char a_star_path1[A_STAR_PATH_SIZE(ASTAR_MAXNODES)];
static unsigned char a_star_path2[A_STAR_PATH_SIZE(ASTAR_MAXNODES)];
static struct a_star_working_space astar_workspace = {
	{ nodeset1, nodeset2 },
	nodemap,
	{ scoremap1, scoremap2 },
	{ a_star_path1, a_star_path2 },
};

/* These wallx[] arrays define internal walls in rooms.  There are pairs of numbers,
 * with a -1 sentinel value  at the end.  The pairs of numbers define horizontal
 * and vertical line segments.  Each column is 16 pixels apart in X, and each row is
 * 16 pixels apart in Y, except the last row and last column, which are 15 pixels
 * from the above or left row or column, respectively. (i.e. last column is 127,
 * last row is 111, or 8 * 16 - 1 or 7 * 16 - 1, respectively.
 *
 *    0  1  2  3  4  5  6  7  8
 *    9 10 11 12 13 14 15 16 17
 *   18 19 20 21 22 23 24 25 26
 *   27 28 29 30 31 32 33 34 35
 *   36 37 38 39 40 41 42 43 44
 *   45 46 47 48 49 50 51 52 53
 *   54 55 56 57 58 59 60 61 62
 *   63 64 65 66 67 68 69 70 71
 *
 * To convert to room coordinates (x -> (0 - 127), y -> (0 - 111), see
 * wall_spec_x() and wall_spec_y() functions.
 *
 */
static const int8_t walls0[] = { 20, 24, 6, 51, 20, 47, 47, 48, 50, 51, -1, };
static const int8_t walls1[] = { 20, 65, 20, 21, 23, 24, 6, 51, -1, };
static const int8_t walls2[] = { 20, 47, 29, 32, 23, 50, -1, };
static const int8_t walls3[] = { 2, 20, 20, 21, 21, 39, 41, 42, 6, 42, 57, 66, -1, };
static const int8_t walls4[] = { 20, 21, 23, 24, 6, 51, 20, 65, -1, };
static const int8_t walls5[] = { 6, 51, 27, 30, 21, 30, 32, 33, 47, 51, -1, };
static const int8_t walls6[] = { 22, 49, 47, 51, -1, };
static const int8_t walls7[] = { 20, 24, 23, 50, 47, 51, -1, };
static const int8_t walls8[] = { 6, 51, 47, 51, 20, 47, 20, 22, -1, };
static const int8_t walls9[] = { 6, 51, 20, 24, 20, 29, 47, 51, -1, };
static const int8_t walls10[] = { 5, 23, 20, 23, 47, 50, 50, 68, -1, };
static const int8_t walls11[] = { 2, 20, 6, 24, 22, 40, 47, 65, 51, 69, -1, };
static const int8_t walls12[] = { 20, 24, 22, 49, -1, };
static const int8_t walls13[] = { 19, 25, 6, 24, 25, 52, 50, 52, 46, 48, 19, 46, -1, };
static const int8_t walls14[] = { 19, 20, 19, 46, 22, 49, 22, 25, 25, 52, 51, 52, -1, };
static const int8_t walls15[] = { 19, 46, 28, 33, 6, 51, -1, };
static const int8_t walls16[] = { 19, 21, 19, 46, 23, 25, 25, 52, 45, 50, -1, };
static const int8_t walls17[] = { 19, 21, 23, 25, 6, 24, 19, 46, 25, 52, 46, 52, -1, };
static const int8_t walls18[] = { 19, 20, 20, 47, 46, 47, 6, 51, -1, };
static const int8_t walls19[] = { 21, 23, 21, 30, 28, 46, 46, 50, 41, 50, -1, };

/* Returns x coord in the range 0 - 127, assuming 0 <= n <= 71 */
static int wall_spec_x(int n)
{
	int x = (n % 9) * 16;
	if (x > 127)
		x = 127;
	return x;
}

/* Returns y coord in the range 0 - 111, assuming 0 <= n <= 71  */
static int wall_spec_y(int n)
{
	int y = 16 * (n / 9);
	if (y > 111)
		y = 111;
	return y;
}

static const int8_t *wall_spec[] = {
	walls0, walls1, walls2, walls3, walls4,
	walls5, walls6, walls7, walls8, walls9,
	walls10, walls11, walls12, walls13, walls14,
	walls15, walls16, walls17, walls18, walls19,
};

#if TARGET_SIMULATOR
static void sanity_check_wall_spec(const int8_t *ws, int n)
{
	int i, x1, y1, x2, y2;
	x2 = 0;
	y2 = 0;

	for (i = 0; ws[i] != -1; i++) {
		x1 = x2;
		y1 = y2;
		x2 = wall_spec_x(ws[i]);
		y2 = wall_spec_y(ws[i]);
		if ((i % 2) != 1 || i == 0)
			continue;
		if (x1 == x2 || y1 == y2)
			continue;
		printf("walls%d contains non-vertical and non-horizontal line segment at %d, (%d, %d)\n",
				n, i - 1, ws[i - 1], ws[i]);
	}
	if ((i % 2) != 0)
		printf("walls%d[] has an odd number of points, %d, should be even.\n", n, i);
}

static void sanity_check_wall_specs(void)
{
	for (size_t i = 0; i < ARRAYSIZE(wall_spec); i++)
		sanity_check_wall_spec(wall_spec[i], i);
}
#else
static inline void sanity_check_wall_specs(void) { } /* compiler will optimize away */
#endif

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
	char interior_walls;
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
static int gulag_text_offset = 0;
static int gulag_text_offset_limit = 425;
static int flag_offset = 0;


struct gulag_stairs_data {
	uint16_t unused;
};

struct gulag_desk_data {
	uint16_t keys:1;
	uint16_t bullets:3;
	uint16_t vodka:1;
	uint16_t locked:1;
	uint16_t safe_combo:1;
	int key;
};

static struct path_data {
#define SOLDIER_PATH_LENGTH 100
	unsigned char pathx[SOLDIER_PATH_LENGTH], pathy[SOLDIER_PATH_LENGTH];
	unsigned char current_step, nsteps;
} path_data[GULAG_MAX_OBJS_PER_ROOM]; /* path data used by soldiers in a room */

struct gulag_soldier_data {
	uint16_t health:3;
	uint16_t spetsnaz:1;
#define SPETSNAZ_ARMBAND BLUE
	uint16_t bullets:3;
	uint16_t grenades:2;
	uint16_t keys:1;
	uint16_t weapon:2;
	uint16_t corpse_direction:1; /* left or right corpse variant */
	uint16_t sees_player_now:1;
	char anim_frame;
	char prev_frame;
	/* Careful, char is *unsigned* by default on the badge! */
	signed char last_seen_x, last_seen_y; /* location player was last seen at */
	unsigned char angle; /* 0 - 127 */
	char state;
#define SOLDIER_STATE_RESTING 0
#define SOLDIER_STATE_MOVING 1
#define SOLDIER_STATE_CHASING 3
#define SOLDIER_STATE_FLEEING 4
#define SOLDIER_STATE_SHOOTING 5
#define SOLDIER_STATE_HANDSUP 6
	unsigned char destx, desty;
	unsigned char path_index;
	unsigned char shoot_cooldown;
};

static const char *safe_documents[] = {
	"INVASION\nPLANS",
	"FIGHTER JET\nPLANS",
	"TANK\nBLUEPRINTS",
	"DEATH RAY\nBLUEPRINTS",
	"LOGISTICS\nPLANS",
	"KOMPROMAT",
	"BOMB PLANS",
	"WARSHIP PLANS",
	"BOMBER PLANS",
};

#define SAFE_DOC_COUNT (sizeof(safe_documents) / sizeof(safe_documents[0]))

struct gulag_safe_data {
	uint8_t documents;
	uint8_t combo[3]; /* R38, L16, R42, or whatever */
	uint8_t opened;
	int angle, first_angle, second_angle;
	uint8_t right_rotations;
	uint8_t left_rotations;
	uint8_t state;
#define COMBO_STATE_WAITING_FOR_FIRST_REV 0
#define COMBO_STATE_WAITING_FOR_SECOND_REV 1
#define COMBO_STATE_GETTING_FIRST_NUMBER 2
#define COMBO_STATE_WAITING_FOR_LEFT_REV 3
#define COMBO_STATE_GETTING_SECOND_NUMBER 4
#define COMBO_STATE_GETTING_THIRD_NUMBER 5
};

struct gulag_chest_data {
	uint16_t bullets:3;
	uint16_t grenades:3;
	uint16_t first_aid:1;
	uint16_t explosives:1;
	uint16_t vodka:1;
	uint16_t potato:1;
	uint16_t cabbage:1;
	uint16_t detonator:1;
	uint16_t locked:1;
	uint16_t opened:1;
	int key;
};

#define MAXLIVEGRENADES 10
static struct grenade {
	short x, y;	/* 8.8 signed fixed point */
	short vx, vy;	/* 8.8 signed fixed point */
	short fuse;	/* ticks until it explodes */
#define GRENADE_FUSE_TIME 90
#define GRENADE_FUSE_TIME_VARIANCE 20
} grenade[MAXLIVEGRENADES];
static int ngrenades = 0;

union gulag_type_specific_data {
	struct gulag_stairs_data stairs;
	struct gulag_desk_data desk;
	struct gulag_soldier_data soldier; /* also used by corpses */
	struct gulag_chest_data chest;
	struct gulag_safe_data safe;
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
#define TYPE_CORPSE 5
#define TYPE_SAFE 6
	union gulag_type_specific_data tsd;
} go[GULAG_MAXOBJS];
int gulag_nobjs = 0;

typedef void (*gulag_object_drawing_function)(struct gulag_object *o);
typedef void (*gulag_object_moving_function)(struct gulag_object *o);

static void draw_stairs_up(struct gulag_object *o);
static void draw_stairs_down(struct gulag_object *o);
static void draw_desk(struct gulag_object *o);
static void draw_soldier(struct gulag_object *o);
static void move_soldier(struct gulag_object *s);
static void draw_corpse(struct gulag_object *o);
static void draw_chest(struct gulag_object *o);
static void draw_grenade(struct grenade *o);
static void draw_safe(struct gulag_object *o);
static void move_grenade(struct grenade *s);

/* Check if a bounding box and a wall segment collide.
 * Assumptions:
 * 1. wall segments are horizontal or vertical, (wx1 == wx2 || wy1 == wy2)
 * 2. bbx1 < bbx2 and bby1 < bby2
 *
 * Returns:
 * 0 if no collision;
 * 1 if collision with horizontal wall
 * 2 if collision with vertical wall
 */
#define HORIZONTAL_WALL_COLLISION 0x01
#define VERTICAL_WALL_COLLISION 0x02
#define HORZVERT_WALL_COLLISION (HORIZONTAL_WALL_COLLISION | VERTICAL_WALL_COLLISION)
static int bbox_wall_collides(int bbx1, int bby1, int bbx2, int bby2, int wx1, int wy1, int wx2, int wy2)
{
	if (wx1 == wx2) { /* vertical wall segment */
		if (wx1 < bbx1 || wx1 > bbx2) /* wall entirely left or right of bb */
			return 0;
		if (wy1 > bby2 && wy2 > bby2) /* wall entirely below bb */
			return 0;
		if (wy1 < bby1 && wy2 < bby1) /* wall entirely above bb */
			return 0;
		return VERTICAL_WALL_COLLISION;
	}
	if (wy1 == wy2) { /* horizontal wall segment */
		if (wy1 < bby1 || wy1 > bby2) /* wall entirely above or below bb */
			return 0;
		if (wx1 < bbx1 && wx2 < bbx1) /* wall entirely left of bb */
			return 0;
		if (wx1 > bbx2 && wx2 > bbx2) /* wall entirely right of bb */
			return 0;
		return HORIZONTAL_WALL_COLLISION;
	}
	return 0;
}

/* See if bounding box (bbx1, bby1) - (bbx2, bby2) collides with any interior walls of
 * the given room.
 * Assumptions:
 * 1: bbx1 < bbx2 and bby1 < bby2
 * 2: bounding box coords are 8.8 fixed point
 *
 * Returns:
 * 0 if no collision
 * 1 if collision with horizontal wall
 * 2 if collision with vertical wall
 * 3 if collision with horizontal and vertical wall
 *
 * If wall_index is not NULL, and a collision occurs, it is filled in with the offset into the room's wall_spec
 * for each wall, and nwallindices will contain the number of wall indices.
 */

static int bbox_interior_wall_collision(struct castle *c, int room, int bbx1, int bby1, int bbx2, int bby2,
		int wall_index[], int *nwallindices)
{
	/* Possible optimization: maybe we unpack wall_spec[n] into a list of coordinates
	 * once when we enter a room, instead of every time we move.  On x86 it might be
	 * faster to unpack each time (in any case, it doesn't matter), on pico, who knows.
	 */
	int capacity = 0;
	int n = c->room[room].interior_walls;
	int nx = 0;
	int rc = 0;
	int accrc = 0;
	if (nwallindices)
		capacity = *nwallindices;
	const int8_t *ws = wall_spec[n];
	for (int i = 0; ws[i] != -1; i += 2) {
		int x1 = wall_spec_x(ws[i]) << 8;
		int y1 = wall_spec_y(ws[i]) << 8;
		int x2 = wall_spec_x(ws[i + 1]) << 8;
		int y2 = wall_spec_y(ws[i + 1]) << 8;
		rc = bbox_wall_collides(bbx1, bby1, bbx2, bby2, x1, y1, x2, y2);
		if (rc) {
			accrc |= rc;
			if (wall_index) {
				if (nx < capacity)
					wall_index[nx] = i;
				nx++;
			}
		}
	}
	if (nwallindices)
		*nwallindices = nx;
	return accrc;
}

/* Returns 1 if the two bounding boxes overlap, otherwise 0.
 * Assumes bb1x1 < bb1x2, bb1y1 < bb1y2, bb2x1 < bb2x2, bb2y1 < bb2y2
 */
static int bb_bb_collision(int bb1x1, int bb1y1, int bb1x2, int bb1y2,
				int bb2x1, int bb2y1, int bb2x2, int bb2y2)
{
	if (bb1x2 < bb2x1) /* bb1 is entirely left of bb2 */
		return 0;
	if (bb1x1 > bb2x2) /* bb1 is entirely right of bb2 */
		return 0;
	if (bb1y2 < bb2y1) /* bb1 is entirely above bb2 */
		return 0;
	if (bb1y1 > bb2y2) /* bb1 is entirely below bb2 */
		return 0;
	return 1; /* bb1 and bb2 must overlap */
}

struct gulag_object_typical_data {
	char w, h; /* width, height of bounding box */
	gulag_object_drawing_function draw;
	gulag_object_moving_function move;
} objconst[] = {
	{ 32, 32, draw_stairs_up, NULL },
	{ 32, 32, draw_stairs_down, NULL },
	{ 16, 8, draw_desk, NULL },
	{ 8, 16, draw_soldier, move_soldier },
	{ 16, 8, draw_chest, NULL },
	{ 16, 8, draw_corpse, NULL, },
	{ 16, 16, draw_safe, NULL, },
};

static const signed char muzzle_xoffset[] = { 7, 7, 6, 6, 0, 0, 0, 5, };
static const signed char muzzle_yoffset[] = { 2, 4, 8, 9, 8, 4, 1, 0, };

static inline void draw_figures_head1(int x, int y)
{
	FbHorizontalLine(x + 3, y, x + 4, y);
	FbHorizontalLine(x + 3, y + 1, x + 4, y + 1);
}

static inline void draw_figures_head2(int x, int y)
{
	FbHorizontalLine(x + 3, y + 1, x + 4, y + 1);
	FbHorizontalLine(x + 3, y + 2, x + 4, y + 2);
}

static inline void draw_figures_vert_body(int x, int y)
{
	FbVerticalLine(x + 3, y + 4, x + 3, y + 15);
	FbVerticalLine(x + 4, y + 4, x + 4, y + 15);
}

/* x, y here are plain screen coords not fixed point 8.8. */
static void draw_figure(int x, int y, int color, int anim_frame, int armband)
{
	FbColor(color);
	switch (anim_frame) {
	case 0:
		draw_figures_head1(x, y);
		FbHorizontalLine(x + 2, y + 3, x + 5, y + 3);
		FbVerticalLine(x + 1, y + 4, x + 1, y + 6);
		FbVerticalLine(x + 6, y + 4, x + 6, y + 6);
		FbVerticalLine(x + 3, y + 4, x + 3, y + 15);
		FbVerticalLine(x + 4, y + 4, x + 4, y + 14);
		FbPoint(x + 5, y + 14);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 1, y + 4);
			FbPoint(x + 6, y + 4);
		}
		break;
	case 1:
		draw_figures_head1(x, y);
		FbHorizontalLine(x + 2, y + 3, x + 5, y + 3);
		FbVerticalLine(x + 1, y + 4, x + 1, y + 6);
		FbVerticalLine(x + 5, y + 4, x + 5, y + 5);
		FbPoint(x + 6, y + 6);
		FbVerticalLine(x + 3, y + 4, x + 3, y + 15);
		FbVerticalLine(x + 4, y + 4, x + 4, y + 14);
		FbPoint(x + 5, y + 14);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 1, y + 4);
			FbPoint(x + 5, y + 4);
		}
		break;
	case 2:
		draw_figures_head2(x, y);
		FbHorizontalLine(x + 2, y + 4, x + 5, y + 4);
		FbVerticalLine(x + 1, y + 5, x + 1, y + 6);
		FbVerticalLine(x + 5, y + 5, x + 5, y + 6);
		FbHorizontalLine(x + 2, y + 7, x + 6, y + 7);
		FbVerticalLine(x + 3, y + 5, x + 3, y + 12);
		FbVerticalLine(x + 4, y + 5, x + 4, y + 9);
		FbPoint(x + 5, y + 10);
		FbHorizontalLine(x, y + 12, x + 3, y + 12);
		FbVerticalLine(x + 6, y + 11, x + 6, y + 15);
		FbPoint(x, y + 13);
		FbPoint(x + 7, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 1, y + 5);
			FbPoint(x + 5, y + 5);
		}
		break;
	case 3:
		draw_figures_head1(x, y);
		FbHorizontalLine(x + 2, y + 3, x + 5, y + 3);
		FbPoint(x + 1, y + 4);
		FbPoint(x, y + 5);
		FbPoint(x + 1, y + 6);
		FbPoint(x + 6, y + 4);
		FbPoint(x + 7, y + 5);
		FbVerticalLine(x + 3, y + 4, x + 3, y + 8);
		FbVerticalLine(x + 4, y + 4, x + 4, y + 8);
		FbLine(x + 2, y + 9, x, y + 14);
		FbLine(x + 5, y + 9, x + 7, y + 11);
		FbVerticalLine(x + 7, y + 12, x + 7, y + 14);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 1, y + 4);
			FbPoint(x + 6, y + 4);
		}
		break;
	case 4:
		draw_figures_head1(x, y);
		FbVerticalLine(x + 3, y + 3, x + 3, y + 8);
		FbVerticalLine(x + 4, y + 3, x + 4, y + 8);
		FbLine(x, y + 5, x + 2, y + 3);
		FbPoint(x, y + 6);
		FbLine(x + 5, y + 4, x + 7, y + 5);
		FbPoint(x + 6, y + 6);
		FbPoint(x + 2, y + 9);
		FbLine(x + 5, y + 9, x + 7, y + 14);
		FbVerticalLine(x + 1, y + 10, x + 1, y + 14);
		FbPoint(x, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 1, y + 4);
			FbPoint(x + 6, y + 4);
		}
		break;
	case 5:
		draw_figures_head2(x, y);
		FbVerticalLine(x + 2, y + 4, x + 2, y + 7);
		FbVerticalLine(x + 3, y + 4, x + 3, y + 9);
		FbVerticalLine(x + 4, y + 4, x + 4, y + 12);
		FbVerticalLine(x + 6, y + 5, x + 6, y + 6);
		FbPoint(x + 1, y + 7);
		FbPoint(x + 5, y + 4);
		FbPoint(x + 5, y + 7);
		FbPoint(x + 2, y + 10);
		FbVerticalLine(x + 1, y + 11, x + 1, y + 14);
		FbHorizontalLine(x + 4, y + 12, x + 7, y + 12);
		FbPoint(x, y + 15);
		FbPoint(x + 7, y + 13);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 2, y + 5);
			FbPoint(x + 2, y + 6);
		}
		break;
	case 6:
		draw_figures_head1(x, y);
		FbVerticalLine(x + 2, y + 3, x + 2, y + 6);
		FbVerticalLine(x + 3, y + 3, x + 3, y + 15);
		FbVerticalLine(x + 4, y + 3, x + 4, y + 15);
		FbVerticalLine(x + 6, y + 4, x + 6, y + 6);
		FbPoint(x + 1, y + 6);
		FbPoint(x + 5, y + 3);
		FbPoint(x + 2, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 2, y + 4);
			FbPoint(x + 6, y + 4);
		}
		break;
	case 7:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbLine(x + 5, y + 4, x + 7, y + 2);
		FbPoint(x + 5, y + 3);
		FbPoint(x + 5, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 5, y + 4);
		}
		break;
	case 8:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbHorizontalLine(x + 5, y + 4, x + 7, y + 4);
		FbPoint(x + 5, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 4, y + 4);
		}
		break;
	case 9:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbLine(x + 5, y + 6, x + 6, y + 7);
		FbPoint(x + 5, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 4, y + 5);
		}
		break;
	case 10:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbLine(x + 2, y + 4, x + 2, y + 8);
		FbVerticalLine(x + 5, y + 4, x + 5, y + 5);
		FbVerticalLine(x + 6, y + 6, x + 6, y + 8);
		FbPoint(x + 5, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 2, y + 5);
			FbPoint(x + 5, y + 5);
		}
		break;
	case 11:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbPoint(x + 2, y + 4);
		FbLine(x + 2, y + 5, x, y + 7);
		FbVerticalLine(x + 5, y + 4, x + 5, y + 8);
		FbPoint(x + 2, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 2, y + 5);
			FbPoint(x + 5, y + 5);
		}
		break;
	case 12:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbHorizontalLine(x, y + 4, x + 2, y + 4);
		FbPoint(x + 2, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 3, y + 4);
		}
		break;
	case 13:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbLine(x, y + 1, x + 2, y + 3);
		FbPoint(x + 2, y + 4);
		FbPoint(x + 2, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 2, y + 4);
		}
		break;
	case 14:
		draw_figures_head2(x, y);
		draw_figures_vert_body(x, y);
		FbLine(x + 2, y + 4, x + 2, y + 8);
		FbVerticalLine(x + 5, y, x + 5, y + 5);
		FbPoint(x + 5, y + 15);
		if (armband) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 2, y + 5);
			FbPoint(x + 5, y + 3);
		}
		break;
	}
}

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

static int astarx_to_8dot8x(int x);
static int astary_to_8dot8y(int x);

static void draw_soldier(struct gulag_object *o)
{
	FbColor(GREEN);
	draw_figure(o->x >> 8, o->y >> 8, GREEN, o->tsd.soldier.anim_frame, o->tsd.soldier.spetsnaz);
#if 0
		FbMove(o->x >> 8, o->y >> 8);
		FbRectangle(objconst[TYPE_SOLDIER].w, objconst[TYPE_SOLDIER].h);
#endif
#if 0
	FbColor(MAGENTA);
	for (int i = 0; i < o->tsd.soldier.nsteps; i++) {
		int x, y;

		x = o->tsd.soldier.pathx[i];
		y = o->tsd.soldier.pathy[i];
		x = astarx_to_8dot8x(x);
		y = astary_to_8dot8y(y);
		x = x >> 8;
		y = y >> 8;
		FbPoint(x, y);
	}
#endif
}

static void draw_corpse(struct gulag_object *o)
{
	int x = o->x >> 8;
	int y = o->y >> 8;

	FbColor(GREEN);
	if (o->tsd.soldier.corpse_direction) { /* left corpse */
		FbHorizontalLine(x, y + 3, x + 1, y + 3);
		FbHorizontalLine(x, y + 4, x + 1, y + 4);
		FbPoint(x + 6, y + 1);
		FbHorizontalLine(x + 3, y + 2, x + 5, y + 2);
		FbPoint(x + 14, y + 2);
		FbHorizontalLine(x + 3, y + 3, x + 14, y + 3);
		FbHorizontalLine(x + 3, y + 4, x + 15, y + 4);
		FbPoint(x + 3, y + 5);
		FbHorizontalLine(x + 4, y + 6, x + 6, y + 6);
		FbColor(RED);
		FbHorizontalLine(x + 5, y + 5, x + 10, y + 5);
		if (o->tsd.soldier.spetsnaz) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 4, y + 2);
			FbPoint(x + 4, y + 6); 
		}
	} else { /* right corpse */
		FbHorizontalLine(x + 14, y + 3, x + 15, y + 3);
		FbHorizontalLine(x + 14, y + 4, x + 15, y + 4);
		FbPoint(x + 9, y + 1);
		FbHorizontalLine(x + 10, y + 2, x + 12, y + 2);
		FbPoint(x + 1, y + 2);
		FbHorizontalLine(x + 1, y + 3, x + 12, y + 3);
		FbHorizontalLine(x, y + 4, x + 12, y + 4);
		FbPoint(x + 12, y + 5);
		FbHorizontalLine(x + 9, y + 6, x + 11, y + 6);
		FbColor(RED);
		FbHorizontalLine(x + 5, y + 5, x + 10, y + 5);
		if (o->tsd.soldier.spetsnaz) {
			FbColor(SPETSNAZ_ARMBAND);
			FbPoint(x + 11, y + 2);
			FbPoint(x + 11, y + 6);
		}
	}
}

static void draw_chest(struct gulag_object *o)
{
	int x, y;

	x = o->x >> 8;
	y = o->y >> 8;

	FbColor(YELLOW);
	FbHorizontalLine(x + 3, y, x + 13, y);
	FbHorizontalLine(x, y + 3, x + 15, y + 3);
	FbLine(x + 3, y, x, y + 3);
	FbLine(x + 13, y, x + 15, y + 3);
	FbVerticalLine(x + 15, y + 3, x + 15, y + 7);
	FbVerticalLine(x, y + 3, x, y + 7);
	FbHorizontalLine(x, y + 7, x + 15, y + 7);

	if (o->tsd.chest.opened) {
		/* open lid */
		FbVerticalLine(x + 3, y - 4, x + 3, y + 3);
		FbVerticalLine(x + 13, y - 4, x + 13, y + 3);
		FbHorizontalLine(x + 3, y - 4, x + 13, y - 4);
	}
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
	return (c->room[room].doors & HAS_LEFT_DOOR) ||
		(room == c->exit_room && get_room_col(room) == 0);
}

static int has_top_door(struct castle *c, int room)
{
	return (c->room[room].doors & HAS_TOP_DOOR) ||
		(room == c->exit_room && get_room_row(room) == 0);
}

static int has_bottom_door(struct castle *c, int room)
{
	if (get_room_row(room) == CASTLE_ROWS - 1)
		return room == c->exit_room;
	return c->room[room + 1].doors & HAS_TOP_DOOR;
}

static int has_right_door(struct castle *c, int room)
{
	if (get_room_col(room) == CASTLE_COLS - 1)
		return room == c->exit_room;
	return c->room[room + CASTLE_ROWS].doors & HAS_LEFT_DOOR;
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
	/* Choose a side of the castle on which to place the exit */
	int row, col, side = random_num(4);
	switch (side) {
	case 0: /* north side */
		row = 0;
		col = random_num(CASTLE_COLS);
		break;
	case 1: /* east side */
		row = random_num(CASTLE_ROWS);
		col = CASTLE_COLS - 1;
		break;
	case 2:
		/* south side */
		row = CASTLE_ROWS - 1;
		col = random_num(CASTLE_COLS);
		break;
	case 3: /* west side */
		row = random_num(CASTLE_ROWS);
		col = 0;
		break;
	}
	c->exit_room = room_no(0, col, row);
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

static int contains_stairs(struct castle *c, int floor, int col, int row)
{
	int room = room_no(floor, col, row);
	for (int i = 0; i < c->room[room].nobjs; i++) {
		int j = c->room[room].obj[i];
		if (go[j].type == TYPE_STAIRS_DOWN || go[j].type == TYPE_STAIRS_UP)
			return 1;
	}
	return 0;
}

static void add_stairs(struct castle *c)
{
	int row, col;

	for (int i = CASTLE_FLOORS - 1; i > 0; i--) {
		/* This loop is to ensure up/down stairs do not overlap on same floor */ 
		do {
			row = random_num(CASTLE_ROWS);
			col = random_num(CASTLE_COLS);
		} while (contains_stairs(c, i, col, row));
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

static void random_location_in_room(int *x, int *y, int width, int height)
{
	*x = random_num((126 - width) << 8);
	*y = random_num((110 - height) << 8);
}

static int add_object_to_room(struct castle *c, int room, int object_type, int x, int y)
{
	int done, w, h;
	int n = gulag_nobjs;
	int bb1x1, bb1y1, bb1x2, bb1y2, bb2x1, bb2y1, bb2x2, bb2y2;

	if (n >= GULAG_MAXOBJS)
		return -1;
	if (c->room[room].nobjs >= GULAG_MAX_OBJS_PER_ROOM)
		return -1;
	go[n].type = object_type;
	go[n].room = room;
	w = objconst[object_type].w;
	h = objconst[object_type].h;

	done = 0;
	do {
		go[n].x = x;
		go[n].y = y;

		bb1x1 = x;
		bb1y1 = y;
		bb1x2 = x + (w << 8);
		bb1y2 = y + (h << 8);

		/* Check against interior wall collisions */
		if (bbox_interior_wall_collision(c, room, bb1x1, bb1y1, bb1x2, bb1y2, NULL, NULL)) {
			random_location_in_room(&x, &y, w, h);
			done = 0;
			continue;
		}
		done = 1;

		/* Check for collisions with other objects already in room */
		for (int i = 0; i < c->room[room].nobjs; i++) {
			int w2, h2;
			int j = c->room[room].obj[i];
			struct gulag_object *o2 = &go[j];
			w2 = objconst[o2->type].w;
			h2 = objconst[o2->type].h;
			bb2x1 = o2->x;
			bb2y1 = o2->y;
			bb2x2 = o2->x + (w2 << 8);
			bb2y2 = o2->y + (h2 << 8);
			if (bb_bb_collision(bb1x1, bb1y1, bb1x2, bb1y2, bb2x1, bb2y1, bb2x2, bb2y2)) {
				random_location_in_room(&x, &y, w, h);
				done = 0;
			}
		}
	} while (!done);

	/* Tell soldier which path data to use */
	if (object_type == TYPE_SOLDIER)
		go[n].tsd.soldier.path_index = c->room[room].nobjs;

	gulag_nobjs++;
	int i = c->room[room].nobjs;
	c->room[room].obj[i] = n;
	c->room[room].nobjs++;
	return n;
}

static void add_desk(struct castle *c, int floor, int has_combo)
{
	int col, row, room;

	do {
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
	go[n].tsd.desk.keys = (random_num(100) < 40);
	go[n].tsd.desk.vodka = (random_num(1000) < 300);
	go[n].tsd.desk.locked = (random_num(100) < 50);
	go[n].tsd.desk.key = random_num(12);
	go[n].tsd.desk.safe_combo = has_combo;
}

static void add_desks(struct castle *c)
{
	for (int i = 0; i < NUM_DESKS; i++) {
		int has_combo = i < difficulty[difficulty_level].num_desk_safe_combo;
		add_desk(c, i % CASTLE_FLOORS, has_combo);
	}
}

static void add_chest(struct castle *c)
{
	int floor, col, row, room;

	do {
		floor = random_num(CASTLE_FLOORS);
		row = random_num(CASTLE_ROWS);
		col = random_num(CASTLE_COLS);
		room = room_no(floor, col, row);
	} while (room_contains_obj_of_type(c, room, TYPE_CHEST));

	int x = random_num(127 - 40) + 8;
	int y = random_num(127 - 16 - 16);

	int n = add_object_to_room(c, room, TYPE_CHEST, x << 8, y << 8);
	if (n < 0)
		return;
	go[n].tsd.chest.bullets = random_num(8);
	if (random_num(1000) < difficulty[difficulty_level].chest_grenade_chance)
		go[n].tsd.chest.grenades = random_num(8);
	go[n].tsd.chest.explosives = (random_num(1000) < 100);
	go[n].tsd.chest.detonator = (random_num(1000) < 100);
	go[n].tsd.chest.vodka = random_num(1000) < 400;
	go[n].tsd.chest.potato = random_num(1000) < 200;
	go[n].tsd.chest.cabbage = random_num(1000) < 200;
	go[n].tsd.chest.first_aid = random_num(1000) < difficulty[difficulty_level].chest_first_aid_chance;
	go[n].tsd.chest.locked = (random_num(100) < 80);
	go[n].tsd.chest.opened = 0;
	go[n].tsd.chest.key = random_num(12);
}

static void add_chests(struct castle *c)
{
	for (int i = 0; i < NUM_CHESTS; i++)
		add_chest(c);
}

static void add_safe(struct castle *c, int floor)
{
	int col, row, room;

	row = random_num(CASTLE_ROWS);
	col = random_num(CASTLE_COLS);
	room = room_no(floor, col, row);

	int x = random_num(127 - 40) + 8;
	int y = random_num(127 - 16 - 16);

	int n = add_object_to_room(c, room, TYPE_SAFE, x << 8, y << 8);
	if (n < 0)
		return;
	go[n].tsd.safe.documents = random_num(SAFE_DOC_COUNT);
	go[n].tsd.safe.combo[0] = random_num(65);
	go[n].tsd.safe.combo[1] = random_num(65);
	go[n].tsd.safe.combo[2] = random_num(65);
	go[n].tsd.safe.opened = 0;
	snprintf(safe_combo[floor], sizeof(safe_combo[floor]), "R%d, L%d, R%d",
		go[n].tsd.safe.combo[0], go[n].tsd.safe.combo[1], go[n].tsd.safe.combo[2]);
}

static void add_safes(struct castle *c)
{
	for (int i = 0; i < CASTLE_FLOORS; i++)
		add_safe(c, i);
}

static void add_soldier_to_room(struct castle *c, int room)
{
	int x, y;

	x = random_num(127 - 10) + 1;
	y = random_num(111 - 18) + 1;
	int n = add_object_to_room(c, room, TYPE_SOLDIER, x << 8, y << 8);
	if (n < 0)
		return;
	go[n].tsd.soldier.health = 1 + random_num(4);
	go[n].tsd.soldier.bullets = random_num(7);
	if (random_num(1000) < 100)
		go[n].tsd.soldier.spetsnaz = 1;
	else
		go[n].tsd.soldier.spetsnaz = 0;
	go[n].tsd.soldier.grenades = random_num(1000) < difficulty[difficulty_level].soldier_grenade_chance;
	go[n].tsd.soldier.keys = random_num(1000) < 200;
	go[n].tsd.soldier.weapon = 0;
	go[n].tsd.soldier.anim_frame = 0;
	go[n].tsd.soldier.prev_frame = 0;
	go[n].tsd.soldier.last_seen_x = -1;
	go[n].tsd.soldier.last_seen_y = -1;
	go[n].tsd.soldier.sees_player_now = 0;
	go[n].tsd.soldier.angle = random_num(128);
	go[n].tsd.soldier.state = SOLDIER_STATE_RESTING;
	go[n].tsd.soldier.shoot_cooldown = difficulty[difficulty_level].soldier_shoot_cooldown;
}

static void add_soldier_to_random_room(struct castle *c)
{
	int f = random_num(CASTLE_FLOORS);
	int col = random_num(CASTLE_COLS);
	int row = random_num(CASTLE_ROWS);
	add_soldier_to_room(c, room_no(f, col, row));
}

static void add_soldiers(struct castle *c)
{
	int s = 0;
	for (int f = 0; f < CASTLE_FLOORS; f++) {
		for (int col = 0; col < CASTLE_COLS; col++) {
			for (int row = 0; row < CASTLE_ROWS; row++) {
				int room = room_no(f, col, row);
				add_soldier_to_room(c, room);
				s++;
			}
		}
	}
	for (; s < difficulty[difficulty_level].nsoldiers; s++)
		add_soldier_to_random_room(c);
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
		c->room[i].interior_walls = random_num(ARRAYSIZE(wall_spec));
	}
	for (i = 0; i < CASTLE_FLOORS; i++)
		add_doors(&castle, i, 0, CASTLE_COLS - 1, 0, CASTLE_ROWS - 1);
	add_start_room(c);
	add_exit_room(c);
	add_stairs(&castle);
	add_desks(&castle);
	add_chests(&castle);
	add_safes(&castle);
	add_soldiers(&castle);
}

#if TARGET_SIMULATOR
static void print_castle_row(struct castle *c, struct player *p, int f, int r)
{
	int room;
	char player_marker;
	char stairs;

	for (int i = 0; i < CASTLE_COLS; i++) {
		room = room_no(f, i, r);
		if (has_top_door(c, room))
			printf("+---  ---");
		else
			printf("+--------");
	}
	printf("+\n");

	for (int i = 0; i < CASTLE_COLS; i++)
		printf("|  %4d  ", room_no(f, i, r));
	printf("|\n");

	for (int i = 0; i < CASTLE_COLS; i++) {
		room = room_no(f, i, r);
		if (room == p->room)
			player_marker = '*';
		else
			player_marker = ' ';
		if (has_left_door(c, room))
			printf("    %c    ", player_marker);
		else
			printf("|   %c    ", player_marker);
	}
	if (has_right_door(c, room))
		printf(" \n");
	else
		printf("|\n");

	for (int i = 0; i < CASTLE_COLS; i++) {
		if (contains_stairs(c, f, i, r))
			stairs = '/';
		else
			stairs = ' ';
		printf("|       %c", stairs);
	}
	printf("|\n");
}

static void print_castle_floor(struct castle *c, struct player *p, int f)
{
	printf("\n\nFLOOR %d\n", f);
	for (int i = 0; i < CASTLE_ROWS; i++)
		print_castle_row(c, p, f, i);
	for (int i = 0; i < CASTLE_COLS; i++) {
		int room = room_no(f, i, CASTLE_ROWS - 1);
		if (has_bottom_door(c, room))
			printf("+---  ---");
		else
			printf("+--------");
	}
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
	p->anim_frame = 0;
	p->prev_frame = 0;
	p->bullets = 10;
	p->grenades = 3;
	p->health = MAX_PLAYER_HEALTH;
	p->keys = 0;
	p->have_war_plans = 0;
	p->search_object = -1;
	p->search_state = search_state_cooldown;
	p->search_timer = 2;
	p->kills = 0;
	p->safecracking_cooldown = 0;
	p->current_safe = -1;
	p->documents_collected = 0;
	p->has_won = 0;
	p->shots_fired = 0;
	p->grenades_thrown = 0;
	p->has_detonator = 0;
	p->has_c4 = 0;
	memset(player.has_combo, 0, sizeof(player.has_combo));
}

static int astarx_to_8dot8x(int x)
{
	const int dx = (120 << 8) / 32;
	const int zx = (2 << 8);

	return zx + x * dx;
}

static int astary_to_8dot8y(int y)
{
	const int dy = ((127 - 16 - 16) << 8) / 32;
	const int zy = (2 << 8);

	return zy + y * dy;
}

static int fpdot8x_to_astarx(int x)
{
	const int dx = (120 << 8) / 32;
	const int zx = (2 << 8);
	x = x - zx;
	if (x < 0)
		x = 0;
	return x / dx;
}

static int fpdot8y_to_astary(int y)
{
	const int dy = ((127 - 16 - 16) << 8) / 32;
	const int zy = (2 << 8);
	y = y - zy;
	if (y < 0)
		y = 0;
	return y / dy;
}

/* given an a-star node (in our case, a pointer into room_cost array),
 * return x y coords into room_cost array */
void astar_node_get_xy(void *node, int *x, int *y)
{
	void *origin = (void *) room_cost;
	*y = (node - origin) / COST_XDIM;
	*x = (node - origin) % COST_XDIM;
}

/* Fill in room_cost[][] upon entry into a room.  It does bounding box checks vs.
 * walls in the room to set things up to allow the soldiers to avoid the
 * walls.  */
static void init_room_cost(struct castle *c, int room)
{
	int x, y, x1, y1, x2, y2, rc;

	for (y = 0; y < COST_YDIM; y++) {
		for (x = 0; x < COST_XDIM; x++) {
			/* Check soldier bounding box against interior walls */
			x1 = astarx_to_8dot8x(x);
			y1 = astary_to_8dot8y(y);
			x2 = x1 + (8 << 8);
			y2 = y1 + (16 << 8);
			rc = bbox_interior_wall_collision(c, room, x1, y1, x2, y2, NULL, NULL);
			if (rc)
				room_cost[y][x] = 255;
			else
				room_cost[y][x] = 0;
		}
	}
}

/* A-star callback, returns manhattan distance between nodes,
 * which the nodes will be pointers into room_cost[][] array
 */
static int distance_fn(__attribute__((unused)) void *context, void *n1, void *n2)
{
	int x1, y1, x2, y2;

	astar_node_get_xy(n1, &x1, &y1);
	astar_node_get_xy(n2, &x2, &y2);
	return abs(x1 - x2) + abs(y1 - y2);
}

/* A-star callback, returns the estimated cost to travel between two nodes. */
static int cost_fn(__attribute__((unused)) void *context, void *n1, void *n2)
{
	unsigned char c1, c2;
	int c;

	/* The nodes will be pointers into room_cost[][] array */
	c1 = *(unsigned char *) n1;
	c2 = *(unsigned char *) n2;
	c = c1 + c2 + 1; /* plus 1 so cost is not zero */
	return c;
}

/* Callback function for A-star to iterate over a node's neighbors */
static void *neighbor_fn(__attribute__((unused)) void *context, void *node, int neighbor)
{
	/* 8 x/y offsets for N, NE, E, SE, S, SW, W, NW */
	const char xo[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	const char yo[] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	int x, y, nx, ny;
	int count = 0;
	unsigned char *n;

	if (neighbor > 7 || neighbor < 0)
		return NULL;
	astar_node_get_xy(node, &x, &y);

	/* Each node has up to eight neighbors, we need to return the right one */
	for (int i = 0; i < 8; i++) {
		nx = x + xo[i];
		ny = y + yo[i];
		if (nx < 0 || nx >= COST_XDIM) /* no neighbor this way */
			continue;
		if (ny < 0 || ny >= COST_YDIM) /* no neighbor this way */
			continue;
		n = &room_cost[ny][nx];
		if (*n == 255) /* wall, not a viable neighbor (aka infinite cost) */
			continue;
		if (count == neighbor) /* viable neighbor node this way */
			return n;
		count++;
	}
	return NULL;
}

static void gulag_init(void)
{
	gulag_nobjs = 0;
	memset(go, 0, sizeof(go));
	init_castle(&castle);
	init_player(&player, castle.start_room);
	init_room_cost(&castle, player.room);
	print_castle(&castle, &player);
	FbInit();
	FbClear();
	gulag_state = GULAG_INTRO;
	screen_changed = 1;
	gulag_text_offset = 0;
	flag_offset = 0;
	sanity_check_wall_specs();
}

static int gulag_text_scroll_lines = 0;
static char **gulag_text_scroll = NULL;

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

static const char *win_text[] = {
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
	"YOU HAVE",
	"SUCCESSFULLY",
	"ESCAPED FROM",
	"THE RUSSIAN",
	"GULAG AND MADE",
	"YOUR WAY BACK",
	"TO FRIENDLY",
	"TERRITORY",
	"AND ARE MET",
	"WITH A HERO'S",
	"WELCOME!",
	"",
	"SLAVA UKRAINI!",
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
	"",
	0,
};


static void gulag_scroll_text(void)
{
	int first_line;
	int y;

	FbColor(YELLOW);

	if (gulag_text_offset == gulag_text_offset_limit) {
		if (player.has_won)
			gulag_state = GULAG_PRINT_STATS;
		else
			gulag_state = GULAG_FLAG;
		return;
	}

	first_line = gulag_text_offset / 10;

	y = -(gulag_text_offset) % 10;
	y = y + 12;
	for (int i = first_line; i < first_line + 14; i++) {
		if (i >= gulag_text_scroll_lines) {
			i++;
			y = y + 10;
			continue;
		}
		if (!gulag_text_scroll[i])
			continue;
		FbMove(5, y);
		FbWriteLine(gulag_text_scroll[i]);
		y = y + 10;
	}
	FbColor(BLACK);
	FbMove(0, 0);
	FbFilledRectangle(128, 10);
	FbColor(BLACK);
	FbMove(0, 140);
	FbFilledRectangle(128, 10);
	gulag_text_offset++;
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		gulag_state = GULAG_START_MENU;
}

static void gulag_intro(void)
{
	gulag_text_scroll = (char **) intro_text;
	gulag_text_scroll_lines = ARRAYSIZE(intro_text);
	gulag_text_offset = 0;
	gulag_text_offset_limit = 425;
	gulag_state = GULAG_SCROLL_TEXT;
}

static void gulag_player_wins(void)
{
	player.has_won = 1;
	player.end_time_ms = rtc_get_ms_since_boot();
	gulag_text_scroll = (char **) win_text;
	gulag_text_scroll_lines = ARRAYSIZE(win_text);
	gulag_text_offset = 0;
	gulag_text_offset_limit = 275;
	gulag_state = GULAG_SCROLL_TEXT;
}

static void gulag_view_combo(void)
{
	FbClear();
	FbMove(3,3);
	FbWriteString("YOU FOUND\nA SLIP OF\nPAPER\n\n");
	FbWriteString("IT LOOKS LIKE A\n");
	FbWriteString("COMBINATION\n");
	FbWriteString("TO A SAFE!\n\n");
	FbWriteString(safe_combo[get_room_floor(player.room)]);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		gulag_state = GULAG_RUN;
	}
}

static void gulag_print_stats(void)
{
	char buf[25];
	int count = 0;
	int hours, mins, secs;
	uint64_t millis;

	FbColor(YELLOW);
        FbMove(3, 3);
	FbWriteString("STATS:\n");
	FbWriteString("YOU KILLED\n");
	snprintf(buf, sizeof(buf), "%d MOBIKS\n", player.kills);
	FbWriteString(buf);
	FbWriteString("SECRET DOCS\n");
	FbWriteString("EXFILTRATED\n");
	for (int i = 0; i < (int) ARRAYSIZE(safe_documents); i++)
		if ((1 << i) & player.documents_collected)
			count++;
	snprintf(buf, sizeof(buf), "  %d / %d\n", count, CASTLE_FLOORS);
	FbWriteString(buf);
	millis = player.end_time_ms - player.start_time_ms;
	secs = millis / 1000;
	mins = secs / 60;
	hours = mins / 60;
	mins = mins - (hours * 60);
	secs = secs - (hours * 60 * 60) - (mins * 60);
	FbWriteString("ELAPSED TIME:\n");
	snprintf(buf, sizeof(buf), "  %02d:%02d:%02d\n", hours, mins, secs);
	FbWriteString(buf);
	FbWriteString("SHOTS FIRED:\n");
	snprintf(buf, sizeof(buf), "  %d\n", player.shots_fired);
	FbWriteString(buf);
	FbWriteString("GRENADES:\n");
	snprintf(buf, sizeof(buf), "  %d\n", player.grenades_thrown);
	FbWriteString(buf);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		gulag_state = GULAG_FLAG;
	}
}

static void gulag_flag(void)
{
	if (flag_offset == 80) {
		flag_offset = 0;
		gulag_state = GULAG_START_MENU;
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
		gulag_state = GULAG_START_MENU;
}

/* When player enters a room, soldiers must forget last seen info */
static void reset_soldier(struct gulag_object *s)
{
	s->tsd.soldier.last_seen_x = -1;
	s->tsd.soldier.last_seen_y = -1;
	s->tsd.soldier.sees_player_now = 0;
}

static void reset_soldiers_in_room(struct castle *c, int room)
{
	for (int i = 0; i < c->room[room].nobjs; i++) {
		int n = c->room[room].obj[i];
		struct gulag_object *o = &go[n];
		if (o->type == TYPE_SOLDIER)
			reset_soldier(o);
	}
}

static void player_enter_room(struct player *p, int room, int x, int y)
{
	p->room = room;
	reset_soldiers_in_room(&castle, room);
	p->x = x;
	p->y = y;
	FbClear();
	screen_changed = 1;
	init_room_cost(&castle, room);
	ngrenades = 0; /* make any live grenades disappear */
#if TARGET_SIMULATOR
	print_castle_floor(&castle, p, get_room_floor(room));
#endif

	for (int i = 0; i < GULAG_MAX_OBJS_PER_ROOM; i++) {
		memset(path_data[i].pathx, 0, sizeof(path_data[i].pathx));
		memset(path_data[i].pathy, 0, sizeof(path_data[i].pathy));
		path_data[i].nsteps = 0;
		path_data[i].current_step = 0;
	}
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
	p->angle = 32; /* down */
	player_enter_room(p, room_no(floor, col, row), stairs->x + (24 << 8), stairs->y + ((32 + 8) << 8));
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
		if (y <= 10 && x > 54 && x < 74 && player.angle > 80 && player.angle < 112) {
			row--;
			if (row < 0) {
				gulag_state = GULAG_PLAYER_WINS;
				return;
			}
			player_enter_room(p, room_no(f, col, row), p->x, (127 - 16 - 12) << 8);
			return;
		}
	}
	if (has_bottom_door(c, room)) {
		if (y >= 127 - 16 - 10 && x > 54 && x < 74 && player.angle > 16 && player.angle < 48) {
			row++;
			if (row >= CASTLE_ROWS) {
				gulag_state = GULAG_PLAYER_WINS;
				return;
			}
			player_enter_room(p, room_no(f, col, row), p->x, 18 << 8);
			return;
		}
	}
	if (has_left_door(c, room)) {
		if (x < 6 && y > 54 && y < 74 && (player.angle > 112 || player.angle < 16)) {
			col--;
			if (col < 0) {
				gulag_state = GULAG_PLAYER_WINS;
				return;
			}
			player_enter_room(p, room_no(f, col, row), 117 << 8, p->y);
			return;
		}
	}
	if (has_right_door(c, room)) {
		if (x >121 && y > 54 && y < 74 && player.angle > 48 && player.angle < 80) {
			col++;
			if (col >= CASTLE_COLS) {
				gulag_state = GULAG_PLAYER_WINS;
				return;
			}
			player_enter_room(p, room_no(f, col, row), 10 << 8, p->y);
			return;
		}
	}
}

static void advance_soldier_animation(struct gulag_object *s)
{
	if (s->tsd.soldier.state == SOLDIER_STATE_RESTING) {
		if (s->tsd.soldier.angle >= 32 && s->tsd.soldier.angle <= 96) /* player is facing to the right */
			s->tsd.soldier.anim_frame = 0;
		else
			s->tsd.soldier.anim_frame = 6; /* left */
		return;
	}
	if (s->tsd.soldier.angle >= 32 && s->tsd.soldier.angle <= 96) { /* soldier is facing to the right */
		switch(s->tsd.soldier.anim_frame) {
		case 0:
			s->tsd.soldier.anim_frame = 1;
			break;
		case 1:
			s->tsd.soldier.anim_frame = 3;
			break;
		case 2:
			s->tsd.soldier.anim_frame = 1;
			break;
		case 3:
			s->tsd.soldier.anim_frame = 2;
			break;
		default:
			s->tsd.soldier.anim_frame = 0;
			break;
		}
	} else { /* player is facing to the left */
		switch(s->tsd.soldier.anim_frame) {
		case 4:
			s->tsd.soldier.anim_frame = 5;
			break;
		case 5:
			s->tsd.soldier.anim_frame = 6;
			break;
		case 6:
			s->tsd.soldier.anim_frame = 4;
			break;
		case 3:
			s->tsd.soldier.anim_frame = 1;
			break;
		default:
			s->tsd.soldier.anim_frame = 6;
			break;
		}
	}
}

static void advance_player_animation(struct player *p)
{
	if (p->angle >= 32 && p->angle <= 96) { /* player is facing to the right */
		switch(p->anim_frame) {
		case 0:
			p->anim_frame = 1;
			break;
		case 1:
			p->anim_frame = 3;
			break;
		case 2:
			p->anim_frame = 1;
			break;
		case 3:
			p->anim_frame = 2;
			break;
		default:
			p->anim_frame = 0;
			break;
		}
	} else { /* player is facing to the left */
		switch(p->anim_frame) {
		case 4:
			p->anim_frame = 5;
			break;
		case 5:
			p->anim_frame = 6;
			break;
		case 6:
			p->anim_frame = 4;
			break;
		case 3:
			p->anim_frame = 1;
			break;
		default:
			p->anim_frame = 6;
			break;
		}
	}
}

/* Returns -1 if no objects collided, or the ID of the object we collided with */
static int player_object_collision(struct castle *c, struct player *p, int newx, int newy)
{
	const int n = c->room[p->room].nobjs;
	int w, h, bb1x1, bb1y1, bb1x2, bb1y2, bb2x1, bb2y1, bb2x2, bb2y2;

	for (int i = 0; i < n; i++) {
		int j = c->room[p->room].obj[i];
		struct gulag_object *o = &go[j];
		int t = o->type;

		switch (t) {
		case TYPE_STAIRS_UP:
			/* Fallthrough */
		case TYPE_STAIRS_DOWN:
			break; /* stairs handled elsewhere */
		case TYPE_DESK: /* Fallthrough */
		case TYPE_CHEST: /* Fallthrough */
		case TYPE_SOLDIER: /* Fallthrough */
		case TYPE_CORPSE:
		case TYPE_SAFE:
			w = objconst[t].w;
			h = objconst[t].h;

			bb1x1 = newx - (4 << 8);
			bb1y1 = newy - (8 << 8);
			bb1x2 = newx + (5 << 8);
			bb1y2 = newy + (9 << 8);

			bb2x1 = o->x;
			bb2y1 = o->y;
			bb2x2 = o->x + (w << 8);
			bb2y2 = o->y + (h << 8);

			if (bb_bb_collision(bb1x1, bb1y1, bb1x2, bb1y2, bb2x1, bb2y1, bb2x2, bb2y2))
				return j;
			break;
		default:
			break;
		}
	}
	return 0;
}

int shooting_frame(struct player *p)
{
	if (p->angle < 8 || p->angle > 119)
		return 12;
	if (p->angle >= 8 && p->angle < 24)
		return 11;
	if (p->angle >= 24 && p->angle < 40)
		return 10;
	if (p->angle >= 40 && p->angle < 56)
		return 9;
	if (p->angle >= 56 && p->angle < 72)
		return 8;
	if (p->angle >= 72 && p->angle < 88)
		return 7;
	if (p->angle >= 88 && p->angle < 104)
		return 14;
	/* if (p->angle >= 104 && p->angle < 120)
		return 11; */
	return 13;
}

static void draw_muzzle_flash(int x, int y, unsigned char angle, int length)
{
	int x2, y2;

	x2 = ((-cosine(angle) * length) >> 8) + x;
	y2 = ((sine(angle) * length) >> 8) + y;
	FbColor(YELLOW);
	if (x >= 0 && y >= 0 && x2 >= 0 && y2 >= 0)
		FbLine(x, y, x2, y2);
}

static void draw_bullet_debris(int x, int y)
{
	int i, x1, y1;

	FbColor(WHITE);
	for (i = 0; i < 10; i++) {
		x1 = x + random_num(16) - 8;
		y1 = y + random_num(16) - 8;
		if (x1 >= 0 && x1 <= 127 && y1 >= 0 && y1 <= 127)
			FbPoint(x1, y1);
	}
}


#define BULLET_SOURCE_PLAYER (-1)
#define BULLET_SOURCE_GRENADE (-2)

static int bullet_track(int x, int y, void *cookie)
{
	intptr_t bullet_source = (intptr_t) cookie;
	int room = player.room;
	int rc;

	if (x < 0 || y < 0 || x > 127 || y > 111) {
		draw_bullet_debris(x, y);
		screen_changed = 1;
		return -1; /* stop bline(), we've left the screen */
	}
	rc = bbox_interior_wall_collision(&castle, room, x << 8, y << 8, (x << 8) + 1, (y << 8) + 1, NULL, NULL);
	if (random_num(100) < 20)
		FbPoint(x, y);
	if (rc) {
		/* We've hit a wall */
		screen_changed = 1;
		draw_bullet_debris(x, y);
		return -1;
	}
	/* Check for collisions with objects in room */
	for (int i = 0; i < castle.room[room].nobjs; i++) {
		int w2, h2;
		int j = castle.room[room].obj[i];
		struct gulag_object *o2 = &go[j];
		w2 = objconst[o2->type].w;
		h2 = objconst[o2->type].h;
		int bb2x1 = o2->x;
		int bb2y1 = o2->y;
		int bb2x2 = o2->x + (w2 << 8);
		int bb2y2 = o2->y + (h2 << 8);
		if (bb_bb_collision(x << 8, y << 8, (x << 8) + 1, (y << 8) + 1, bb2x1, bb2y1, bb2x2, bb2y2)) {
			switch (go[j].type) {
			case TYPE_STAIRS_UP:
				return 0;
			case TYPE_STAIRS_DOWN:
				return 0;
			case TYPE_DESK:
				draw_bullet_debris(x, y);
				if (random_num(100) < 50)
					go[j].tsd.desk.locked = 0;
				break;
			case TYPE_SOLDIER:
				if (bullet_source > 0 && bullet_source == j) /* bullet was fired by this soldier */
					return 0; /* soldiers do not shoot themselves */
				draw_bullet_debris(x, y);
				go[j].tsd.soldier.health--;

				if (go[j].tsd.soldier.health == 0) {

					/* This will need fixing if soldiers can throw grenades */
					if (bullet_source == BULLET_SOURCE_PLAYER ||
						bullet_source == BULLET_SOURCE_GRENADE)
						player.kills++;

					go[j].type = TYPE_CORPSE;
					if ((go[j].x >> 8) > 64) {
						go[j].tsd.soldier.corpse_direction = 1;
						go[j].x -= (8 << 8);
						go[j].y += (8 << 8);
					} else {
						go[j].tsd.soldier.corpse_direction = 0;
						go[j].y += (8 << 8);
					}
				}
				break;
			case TYPE_CHEST:
				draw_bullet_debris(x, y);
				if (random_num(100) < 50)
					go[j].tsd.chest.locked = 0;
				break;
			case TYPE_CORPSE: /* bullets don't hit corpses because they are on the floor */
				return 0;
			default:
				break;
			}
			return -1;
		}
	}
	/* Check for collisions with the player */
	if (bullet_source != BULLET_SOURCE_PLAYER) {
		int bbx1, bby1, bbx2, bby2;

		bbx1 = player.x - (3 << 8);
		bby1 = player.y - (6 << 8);
		bbx2 = player.x + (3 << 8);
		bby2 = player.y + (6 << 8);

		if (bb_bb_collision(x << 8, y << 8, (x << 8) + 1, (y << 8) + 1, bbx1, bby1, bbx2, bby2)) {
			if (player.health > 0)
				player.health = player.health - difficulty[difficulty_level].bullet_damage;
			if ((player.health >> 8) <= 0) {
				printf("PLAYER IS DEAD!\n");
			}
			return -1; /* stop the bullet */
		}
	}

	screen_changed = 1;
	return 0;
}

static void fire_bullet(__attribute__((unused)) struct player *p, int x, int y, int angle, int bullet_source)
{
	int tx, ty;

	draw_muzzle_flash(x, y, angle, 5);
	tx = ((-cosine(angle) * 400) >> 8) + x; /* 400 will put target tx,ty offscreen, guaranteed */
	ty = ((sine(angle) * 400) >> 8) + y;
	bline(x, y, tx, ty, bullet_track, (void *) (intptr_t) bullet_source);
}

static void fire_gun(struct player *p)
{
	int muzzle_flash_index;
	int x, y; // , tx, ty;

	p->shots_fired++;
	p->anim_frame = shooting_frame(p);
	muzzle_flash_index = p->anim_frame - 7;
	x = (p->x >> 8) - 4 + muzzle_xoffset[muzzle_flash_index];
	y = (p->y >> 8) - 8 + muzzle_yoffset[muzzle_flash_index];
	fire_bullet(p, x, y, p->angle, BULLET_SOURCE_PLAYER);
#if 0
	draw_muzzle_flash(x, y, p->angle, 5);
	tx = ((-cosine(p->angle) * 400) >> 8) + x; /* 400 will put target tx,ty offscreen, guaranteed */
	ty = ((sine(p->angle) * 400) >> 8) + y;
	bline(x, y, tx, ty, bullet_track, p);
#endif
}

static void remove_grenade(struct grenade *g)
{
	if (ngrenades <= 0)
		return;
	int n = g - &grenade[0];
	size_t bytes = (ngrenades - n - 1) * sizeof(*g);
	if (bytes > 0)
		memmove(g, g + 1, bytes);
	ngrenades--;
}

static void throw_grenade(struct player *p)
{
	if (ngrenades >= MAXLIVEGRENADES)
		return;
	p->grenades_thrown++;
	int n = ngrenades;
	grenade[n].x = player.x;
	grenade[n].y = player.y;
	grenade[n].vx = (grenade_speed * -cosine(p->angle)) >> 8;
	grenade[n].vy = (grenade_speed * sine(p->angle)) >> 8;
	grenade[n].fuse = GRENADE_FUSE_TIME + random_num(GRENADE_FUSE_TIME_VARIANCE);
	ngrenades++;
}

#define DEBUG_SEARCH 0
#if DEBUG_SEARCH
#define debug_search(...) printf(__VA_ARGS__)

static const char *search_state_name[] = {
	"IDLE",
	"SEARCHING",
	"TRYING KEYS",
	"UNLOCKED",
	"NO DICE",
	"LOCKED",
	"FINDING ITEMS",
	"COOLDOWN",
};

#else
#define debug_search(...)
#endif

static void set_search_state(enum search_state s, int time)
{
	debug_search("search state now: %s, time = %d\n", search_state_name[s], time);
	player.search_state = s;
	player.search_timer = time;
}

static void goto_search_cooldown(void)
{
	set_search_state(search_state_cooldown, SEARCH_COOLDOWN_TIME);
	strcpy(player_message, "");
	display_message = 0;
	return;
}

struct search_item {
	char name[25];
};

static void maybe_search_for_loot(void)
{
	char *object_type;
	int distance2, dx, dy;
	struct gulag_object *o;
	int locked = 0;
	struct search_item search_item_list[10];
	int nitems, took_item;

	screen_changed = 1;

	if (player.safecracking_cooldown > 0)
		player.safecracking_cooldown--;

	if (player.search_object == -1)
		o = NULL;
	else
		o = &go[player.search_object];

	switch (player.search_state) {
	case search_state_idle:
		if (player.search_object == -1)
			return;
		dx = (o->x >> 8) - (player.x >> 8);
		dy = (o->y >> 8) - (player.y >> 8);
		distance2 = dx * dx + dy * dy;
		if (distance2 > MIN_SEARCH_DIST2) {
			player.search_object = -1;
			goto_search_cooldown();
			return;
		}
		if (player.search_timer > 0) {
			player.search_timer--;
			if (player.search_timer == 0)
				set_search_state(search_state_searching, SEARCH_TIME);
		}
		return;
	case search_state_searching:
		if (player.search_object == -1) {
			goto_search_cooldown();
			return;
		}
		switch (o->type) {
		case TYPE_DESK:
			object_type = "DESK";
			locked = o->tsd.desk.locked;
			break;
		case TYPE_CHEST:
			object_type = "CHEST";
			locked = o->tsd.chest.locked;
			break;
		case TYPE_CORPSE:
			object_type = "BODY";
			locked = 0;
			break;
		default:
			object_type = "...";
			break;
		}
		snprintf(player_message, sizeof(player_message), "SEARCHING %s", object_type);
		display_message = 1;
		if (player.search_timer > 0)
			player.search_timer--;
		if (player.search_timer == 0) {
			if (!locked) {
				set_search_state(search_state_finding_items, SEARCH_ITEM_TIME);
				player.search_item_num = 0;
				return;
			}
			if (player.keys > 0) {
				set_search_state(search_state_trying_keys, SEARCH_KEY_TIME);
				player.trying_key = 0;
				return;
			}
			set_search_state(search_state_locked, SEARCH_LOCKED_TIME);
			return;
		}
		return;
	case search_state_locked:
		if (player.search_object == -1) {
			goto_search_cooldown();
			return;
		}
		snprintf(player_message, sizeof(player_message), "LOCKED!");
		display_message = 1;
		if (player.search_timer > 0)
			player.search_timer--;
		if (player.search_timer == 0) {
			goto_search_cooldown();
			return;
		}
		return;
	case search_state_trying_keys:
		if (player.search_object == -1) {
			goto_search_cooldown();
			return;
		}
		snprintf(player_message, sizeof(player_message), "TRYING KEY %d/%d", player.trying_key, player.keys);
		display_message = 1;
		if (player.search_timer > 0)
			player.search_timer--;
		if (player.search_timer == 0) {
			switch (o->type) {
			case TYPE_DESK:
				if (player.trying_key == o->tsd.desk.key) {
					set_search_state(search_state_unlocked, SEARCH_LOCKED_TIME);
					o->tsd.desk.locked = 0;
					player.trying_key = 0;
					return;
				}
				break;
			case TYPE_CHEST:
				if (player.trying_key == o->tsd.chest.key) {
					set_search_state(search_state_unlocked, SEARCH_LOCKED_TIME);
					o->tsd.chest.locked = 0;
					player.trying_key = 0;
					return;
				}
				break;
			}
			player.trying_key++;
			if (player.trying_key >= player.keys) { /* tried all the keys already */
				set_search_state(search_state_no_dice, SEARCH_NO_DICE_TIME);
				return;
			}
			set_search_state(search_state_trying_keys, SEARCH_KEY_TIME);
		}
		return;
	case search_state_no_dice:
		if (player.search_object == -1) {
			goto_search_cooldown();
			return;
		}
		snprintf(player_message, sizeof(player_message), "NO DICE!");
		display_message = 1;
		if (player.search_timer > 0)
			player.search_timer--;
		if (player.search_timer == 0) {
			goto_search_cooldown();
			return;
		}
		return;
	case search_state_unlocked:
		if (player.search_object == -1) {
			goto_search_cooldown();
			return;
		}
		snprintf(player_message, sizeof(player_message), "UNLOCKED!");
		display_message = 1;
		if (player.search_timer > 0)
			player.search_timer--;
		if (player.search_timer == 0) {
			set_search_state(search_state_finding_items, SEARCH_ITEM_TIME);
			player.search_item_num = 0;
			return;
		}
		return;
	case search_state_finding_items:
		took_item = 0;
		if (player.search_object == -1) {
			goto_search_cooldown();
			return;
		}
		/* Build the list of items we found */
		nitems = 0;
		if (player.search_timer > 0)
			player.search_timer--;
		switch (o->type) {
		case TYPE_DESK:
			if (o->tsd.desk.vodka > 0) {
				strcpy(search_item_list[nitems].name, "FLASK OF VODKA");
				display_message = 1;
				if (player.search_item_num == nitems && player.search_timer == 0) {
					o->tsd.desk.vodka = 0;
					took_item = 1;
					/* TODO: stash the vodka on the player for future use */
				}
				nitems++;
			}
			if (o->tsd.desk.keys > 0) {
				strcpy(search_item_list[nitems].name, "A KEY");
				display_message = 1;
				if (player.search_item_num == nitems && player.search_timer == 0) {
					player.keys += o->tsd.desk.keys;
					o->tsd.desk.keys = 0;
					took_item = 1;
				}
				nitems++;
			}
			if (o->tsd.desk.bullets > 0) {
				snprintf(search_item_list[nitems].name, sizeof(search_item_list[nitems].name),
					"%d %s", o->tsd.desk.bullets, o->tsd.desk.bullets > 1 ? "BULLETS" : "BULLET");
				if (player.search_item_num == nitems && player.search_timer == 0) {
					player.bullets += o->tsd.desk.bullets;
					o->tsd.desk.bullets = 0;
					took_item = 1;
				}
				nitems++;
			}
			if (o->tsd.desk.safe_combo > 0) {
				snprintf(search_item_list[nitems].name, sizeof(search_item_list[nitems].name),
					"%s", "SLIP OF PAPER");
				if (player.search_item_num == nitems && player.search_timer == 0) {
					o->tsd.desk.safe_combo = 0;
					took_item = 1;
					player.has_combo[get_room_floor(player.room)] = 1;
					gulag_state = GULAG_VIEW_COMBO;
				}
				nitems++;
			}
			break;
		case TYPE_CHEST:
			o->tsd.chest.opened = 1;
			if (o->tsd.chest.grenades > 0) {
				snprintf(search_item_list[nitems].name, sizeof(search_item_list[nitems].name),
					"%d %s", o->tsd.chest.grenades, o->tsd.chest.grenades > 1 ? "GRENADES" : "GRENADE");
				if (player.search_item_num == nitems && player.search_timer == 0) {
					player.grenades += o->tsd.chest.grenades;
					o->tsd.chest.grenades = 0;
					took_item = 1;
				}
				nitems++;
			}
			if (o->tsd.chest.explosives > 0) {
				if (!took_item) {
					strcpy(search_item_list[nitems].name, "C-4 EXPLOSIVE");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						o->tsd.chest.explosives = 0;
						player.has_c4 = 1;
						took_item = 1;
					}
				}
				nitems++;
			}
			if (o->tsd.chest.detonator > 0) {
				if (!took_item) {
					strcpy(search_item_list[nitems].name, "DETONATOR");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						o->tsd.chest.detonator = 0;
						player.has_detonator = 1;
						took_item = 1;
					}
				}
				nitems++;
			}
			if (o->tsd.chest.bullets > 0) {
				if (!took_item) {
					snprintf(search_item_list[nitems].name, sizeof(search_item_list[nitems].name),
						"%d %s", o->tsd.chest.bullets, o->tsd.chest.bullets > 1 ? "BULLETS" : "BULLET");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						player.bullets += o->tsd.chest.bullets;
						o->tsd.chest.bullets = 0;
						took_item = 1;
					}
				}
				nitems++;
			}
			if (o->tsd.chest.vodka > 0) {
				if (!took_item) {
					strcpy(search_item_list[nitems].name, "FLASK OF VODKA");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						o->tsd.chest.vodka = 0;
						took_item = 1;
						player.health += VODKA_HEALTH_BOOST;
						if (player.health > MAX_PLAYER_HEALTH)
							player.health = MAX_PLAYER_HEALTH;
					}
				}
				nitems++;
			}
			if (o->tsd.chest.potato > 0) {
				if (!took_item) {
					strcpy(search_item_list[nitems].name, "A POTATO");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						o->tsd.chest.potato = 0;
						took_item = 1;
						player.health += POTATO_HEALTH_BOOST;
						if (player.health > MAX_PLAYER_HEALTH)
							player.health = MAX_PLAYER_HEALTH;
					}
				}
				nitems++;
			}
			if (o->tsd.chest.cabbage > 0) {
				if (!took_item) {
					strcpy(search_item_list[nitems].name, "A CABBAGE");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						o->tsd.chest.cabbage = 0;
						took_item = 1;
						player.health += CABBAGE_HEALTH_BOOST;
						if (player.health > MAX_PLAYER_HEALTH)
							player.health = MAX_PLAYER_HEALTH;
					}
				}
				nitems++;
			}
			if (o->tsd.chest.first_aid > 0) {
				if (!took_item) {
					strcpy(search_item_list[nitems].name, "FIRST AID KIT");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						o->tsd.chest.first_aid = 0;
						took_item = 1;
						player.health += FIRST_AID_HEALTH_BOOST;
						if (player.health > MAX_PLAYER_HEALTH)
							player.health = MAX_PLAYER_HEALTH;
					}
				}
				nitems++;
			}
			break;
		case TYPE_CORPSE:
			if (o->tsd.soldier.grenades > 0) {
				snprintf(search_item_list[nitems].name, sizeof(search_item_list[nitems].name),
					"%d %s", o->tsd.soldier.grenades, o->tsd.soldier.grenades > 1 ? "GRENADES" : "GRENADE");
				if (player.search_item_num == nitems && player.search_timer == 0) {
					player.grenades += o->tsd.soldier.grenades;
					o->tsd.soldier.grenades = 0;
					took_item = 1;
				}
				nitems++;
			}
			if (o->tsd.soldier.bullets > 0) {
				if (!took_item) {
					snprintf(search_item_list[nitems].name, sizeof(search_item_list[nitems].name),
						"%d %s", o->tsd.soldier.bullets, o->tsd.soldier.bullets > 1 ? "BULLETS" : "BULLET");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						player.bullets += o->tsd.soldier.bullets;
						o->tsd.soldier.bullets = 0;
						took_item = 1;
					}
				}
				nitems++;
			}
			if (o->tsd.soldier.keys > 0) {
				if (!took_item) {
					strcpy(search_item_list[nitems].name, "A KEY");
					if (player.search_item_num == nitems && player.search_timer == 0) {
						player.keys += o->tsd.soldier.keys;
						o->tsd.soldier.keys = 0;
						took_item = 1;
					}
				}
				nitems++;
			}
			break;
		}
		if (took_item && nitems > 1) {
			/* we took an item, but more items remain. */
			player.search_timer = SEARCH_ITEM_TIME;
		}
		if (nitems == 0) {
			strcpy(player_message, "NOTHING.");
			display_message = 1;
			debug_search("NOTHING.\n");
		} else {
			strcpy(player_message, search_item_list[player.search_item_num].name);
			display_message = 1;
			debug_search("%s\n", player_message);
		}
		if (player.search_timer == 0) {
			if (nitems == 0) {
				goto_search_cooldown();
				return;
			}
			player.search_item_num++;
			if (player.search_item_num >= nitems) {
				goto_search_cooldown();
				return;
			}
		}
		return;
	case search_state_cooldown:
		if (player.search_timer > 0)
			player.search_timer--;
		if (player.search_timer == 0)
			set_search_state(search_state_idle, SEARCH_INIT_TIME);
		return;
	}
}

static void maybe_crack_safe(struct gulag_object *safe)
{
	/* Wait some time between safecracking attempts */
	if (player.safecracking_cooldown > 0)
		return;

	/* Can't crack a safe if it's already open */
	if (safe->tsd.safe.opened)
		return;

	/* Not safe to attempt safecracking if soldiers are running around here */
	for (int i = 0; i < castle.room[player.room].nobjs; i++) {
		int n = castle.room[player.room].obj[i];
		if (go[n].type == TYPE_SOLDIER)
			return;
	}
	gulag_state = GULAG_SAFECRACKING;
	player.current_safe = safe - &go[0];
	safe->tsd.safe.right_rotations = 0;
	safe->tsd.safe.left_rotations = 0;
	safe->tsd.safe.angle = random_num(128);
	safe->tsd.safe.first_angle = safe->tsd.safe.angle;
	safe->tsd.safe.second_angle = safe->tsd.safe.angle;
	safe->tsd.safe.state = COMBO_STATE_WAITING_FOR_FIRST_REV;
}

static void check_buttons()
{
	static int firing_timer = 0;
	int down_latches = button_down_latches();
	int rotary_switch = button_get_rotation(0);
	int anything_pressed = 0;
	int n;

	if (rotary_switch) {
		short new_angle = player.angle - 3 * rotary_switch;
		if (new_angle > 127)
			new_angle -= 127;
		if (new_angle < 0)
			new_angle += 127;
		player.oldangle = player.angle;
		player.angle = (unsigned char) new_angle;
		screen_changed = 1;
		anything_pressed = 1;
	}

	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		if (player.bullets > 0) {
			fire_gun(&player);
			firing_timer = 10; /* not sure this will work on pico */
			screen_changed = 1;
			anything_pressed = 1;
			player.bullets--;
		}
	}
        if (BUTTON_PRESSED(BADGE_BUTTON_SW2, down_latches)) {
		if (player.grenades > 0) {
			throw_grenade(&player);
			firing_timer = 10;
			screen_changed = 1;
			anything_pressed = 1;
			player.grenades--;
		}
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_A, down_latches)) {
		gulag_state = GULAG_MAYBE_EXIT;
        }
	if (button_poll(BADGE_BUTTON_LEFT)) {
		short new_angle = player.angle + 3;
		if (new_angle > 127)
			new_angle -= 127;
		player.oldangle = player.angle;
		player.angle = (unsigned char) new_angle;
		screen_changed = 1;
		anything_pressed = 1;
	}
	if (button_poll(BADGE_BUTTON_RIGHT)) {
		short new_angle = player.angle - 3;
		if (new_angle < 0)
			new_angle += 127;
		player.oldangle = player.angle;
		player.angle = (unsigned char) new_angle;
		screen_changed = 1;
		anything_pressed = 1;
	}
	if (button_poll(BADGE_BUTTON_UP) || button_poll(BADGE_BUTTON_ENCODER_A)) {
		anything_pressed = 1;
		int newx, newy;
		newx = ((-cosine(player.angle) * player_speed) >> 8) + player.x;
		newy = ((sine(player.angle) * player_speed) >> 8) + player.y;

		/* Check for exterior wall collision ... */
		if (newx < 5 << 8)
			newx = 5 << 8;
		if (newx > ((127 - 4) << 8))
			newx = (127 - 4) << 8;
		if (newy < 9 << 8)
			newy = 9 << 8;
		if (newy > ((127 -16 - 8) << 8))
			newy = (127 - 16 - 8) << 8;

		/* Check for interior wall collision */
		int wall_index[2], nwallindices;
		nwallindices = 2;
		(void) bbox_interior_wall_collision(&castle, player.room,
				newx - (4 << 8), newy - (9 << 8),
				newx + (5 << 8), newy + (8 << 8), wall_index, &nwallindices);
		for (int j = 0; j < nwallindices; j++) {
			int n = castle.room[player.room].interior_walls;
			const int8_t *ws = wall_spec[n];
			int x1 = wall_spec_x(ws[wall_index[j]]);
			int y1 = wall_spec_y(ws[wall_index[j]]);
			int x2 = wall_spec_x(ws[wall_index[j] + 1]);
			int y2 = wall_spec_y(ws[wall_index[j] + 1]);
			if (x1 == x2) { /* vertical wall */
				if (x1 < (player.x >> 8)) { /* wall is to left of player */
					newx = (x1 + 8) << 8;
				} else {			/* wall is to the right of player */
					newx = (x1 - 8) << 8;
				}
			} else if (y1 == y2) { /* horizontal wall */
				if (y1 < (player.y >> 8)) { /* wall is above player */
					newy = (y1 + 10) << 8;
				} else {			/* wall is below player */
					newy = (y1 - 10) << 8;
				}
			}
		}

		n = player_object_collision(&castle, &player, newx, newy);
		if (n >= 0) { /* Collision with some object... */
			switch (go[n].type) {
			case TYPE_STAIRS_UP: /* fallthrough */
			case TYPE_STAIRS_DOWN:  /* fallthrough */
				break; /* handled elsewhere */
			/* Chests and desks don't block movement so player isn't trapped by furniture. */
			case TYPE_CHEST:
				player.search_object = n;
				break;
			case TYPE_DESK:
				player.search_object = n;
				break;
			case TYPE_CORPSE:
				player.search_object = n;
				break;
			case TYPE_SOLDIER: /* Soldiers block player movement */
				/* Here, maybe trigger soldier action? */
				break;
			case TYPE_SAFE:
				maybe_crack_safe(&go[n]);
				break;
			default:
				break;
			}
		} else {
			player.search_object = -1;
			display_message = 0;
		}
		/* if (player.search_object != -1 && player.search_state == search_state_idle)
			set_search_state(search_state_searching, SEARCH_TIME); */

		player.oldx = player.x;
		player.oldy = player.y;
		player.x = (short) newx;
		player.y = (short) newy;
		screen_changed = 1;
		check_doors(&castle, &player);
		if (gulag_state != GULAG_RUN)
			return;
		check_object_collisions(&castle, &player);
		advance_player_animation(&player);
	}
	if (button_poll(BADGE_BUTTON_DOWN)) {
	}

	if (!anything_pressed) { /* nothing pressed */
		if (firing_timer > 0) {
			if (firing_timer == 10) /* not sure this will work on pico */
				screen_changed = 1; /* cause muzzle flash to disappear */
			firing_timer--;
		} else if (player.anim_frame != 0 && player.anim_frame != 6) {
			if (player.angle >= 32 && player.angle <= 96) /* player is facing to the right */
				player.anim_frame = 0;
			else
				player.anim_frame = 6; /* left */
			screen_changed = 1;
		}
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

#if 0
static void erase_player(struct player *p)
{
	short x, y;
	int dx, dy;

	x = p->oldx >> 8;
	y = p->oldy >> 8;

	draw_figure(x, y, BLACK, p->prev_frame, 0);

	dx = ((-cosine(p->oldangle) * 16 * player_speed) >> 8) + p->oldx;
	dy = ((sine(p->oldangle) * 16 * player_speed) >> 8) + p->oldy;
	x = (short) (dx >> 8);
	y = (short) (dy >> 8);
	FbColor(BLACK);
	draw_plus(x, y);
}
#endif

static void draw_player(struct player *p)
{
	short x, y;
	int dx, dy;

	// erase_player(p);
	x = p->x >> 8;
	y = p->y >> 8;

	FbColor(WHITE);
	draw_figure(x - 4, y - 8, WHITE, p->anim_frame, 0);
#if 0
	/* Draw bounding box (debug) */
	FbColor(GREEN);
	FbHorizontalLine(x - 4, y - 8, x + 5, y - 8);
	FbHorizontalLine(x - 4, y + 9, x + 5, y + 9);
	FbVerticalLine(x - 4, y - 8, x - 4, y + 9);
	FbVerticalLine(x + 5, y - 8, x + 5, y + 9);
#endif

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

static void draw_interior_walls(struct castle *c, int room)
{
	/* Draw interior walls */
	int n = c->room[room].interior_walls;
	const int8_t *ws =  wall_spec[n];

	for (int i = 0; ws[i] != -1; i += 2) {
		int x1 = wall_spec_x(ws[i]);
		int y1 = wall_spec_y(ws[i]);
		int x2 = wall_spec_x(ws[i + 1]);
		int y2 = wall_spec_y(ws[i + 1]);
		if (x1 == x2)
			FbVerticalLine(x1, y1, x1, y2);
		else
			FbHorizontalLine(x1, y1, x2, y1);
	}
}

#if 0
static void draw_cost_dots(void)
{
	int dx, dy, px, py;

	dx = (120 << 8) / 32;
	dy = ((127 - 16 - 16) << 8) / 32;

	px = (2 << 8);
	py = (2 << 8);
	FbColor(MAGENTA);
	for (int y = 0; y < COST_YDIM; y++) {
		for (int x = 0; x < COST_XDIM; x++) {
			if (room_cost[y][x])
				FbColor(RED);
			else
				FbColor(GREEN);
			FbPoint(px >> 8, py >> 8);
			px += dx;
		}
		px = (2 << 8);
		py += dy;
	}
}
#endif

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
	draw_interior_walls(c, room);
	/* draw_cost_dots(); */
	draw_room_objs(c, room);
}

static void draw_grenades(void)
{
	for (int i = 0; i < ngrenades; i++)
		draw_grenade(&grenade[i]);
}

static void draw_safe(struct gulag_object *safe)
{
	FbColor(CYAN);
	FbMove(safe->x >> 8, safe->y >> 8);
	FbRectangle(16, 16);
	FbMove((safe->x >> 8) + 3, (safe->y >> 8) + 3);
	FbRectangle(10, 10);
}

static void draw_player_data(void)
{
	char buf[100];
	int blink = 0;

	if (display_message) {
		FbColor(YELLOW);
		FbMove(3, 130);
		FbWriteString(player_message);
		return;
	}

	if ((player.health >> 8) < 15) {
		FbColor(RED);
		blink = ((game_timer & 0x0C) == 0);
	} else {
		if ((player.health >> 8) < 50)
			FbColor(YELLOW);
		else
			FbColor(GREEN);
	}
	snprintf(buf, sizeof(buf), "H:%4d", player.health >> 8);
	if (!blink) {
		FbMove(3, 130);
		FbWriteString(buf);
	}
	FbColor(YELLOW);
	snprintf(buf, sizeof(buf), "B:%4d", player.bullets);
	FbMove(3, 140); FbWriteString(buf);
	snprintf(buf, sizeof(buf), "G:%4d", player.grenades);
	FbMove(3, 150); FbWriteString(buf);
	snprintf(buf, sizeof(buf), "K:%4d", player.keys);
	FbMove(64, 130); FbWriteString(buf);
}

static void maybe_draw_wasted(void)
{
	int x, y;

	if (player.health > 0)
		return;

	gulag_state = GULAG_PLAYER_DIED;
	x = player.x >> 8;
	if (x < 3)
		x = 3;
	if (x > LCD_XSIZE - 8 * 7)
		x = LCD_XSIZE - 8 * 7;
	y = player.y >> 8;
	if (y < 8)
		y = 8;

	FbMove(x, y);
	FbColor(WHITE);
	FbWriteString("WASTED!");
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	draw_room(&castle, player.room);
	draw_player(&player);
	draw_grenades();
	draw_player_data();
	maybe_draw_wasted();
	FbSwapBuffers();
	screen_changed = 0;
}

struct soldier_eyeline_data {
	struct player *p;
	struct gulag_object *s;
	char seen;
	int target_x, target_y;
};

static int soldier_eyeline(int x, int y, void *cookie)
{
	struct soldier_eyeline_data *sed = cookie;
	int bbx1, bby1, bbx2, bby2;

	bbx1 = (x <<  8);
	bby1 = (y << 8);
	bbx2 = (x << 8) + 1;
	bby2 = (y << 8) + 1;

#if 0
	/* Draw eyelines for debugging */
	FbColor(YELLOW);
	FbPoint(x, y);
#endif

	if (bbox_interior_wall_collision(&castle, sed->p->room, bbx1, bby1, bbx2, bby2, NULL, NULL)) {
		sed->seen = 0;
		return -1;
	}
	if (x == sed->target_x && y == sed->target_y)
		sed->seen = 1;
	return 0;
}

static void soldier_look_for_player(struct gulag_object *s, struct player *p)
{
	struct soldier_eyeline_data head_data, foot_data;

	if (s->tsd.soldier.last_seen_x == -1)	/* If we haven't seen player yet then there's */
		if (random_num(100) < 90)	/* a 90% chance we won't notice him in this moment */
			return;			/* Saves looking all the time, and we'll see player */
						/* soon enough. */

	head_data.s = s;
	head_data.p = p;
	head_data.seen = 0;
	foot_data.s = s;
	foot_data.p = p;
	foot_data.seen = 0;

	/* TODO: if we had a fixed point dot product, we could dot the
	 * soldier's facing direction with a vector to the player and
	 * skip looking for the player when the dot product was negative
	 * or close to zero. But to get the unit vector towards the player
	 * we need a fixed point square root.
	 */

	int soldier_x = (s->x >> 8) + 4;
	int soldier_heady = (s->y >> 8) + 1;

	int player_x = (p->x >> 8);
	int player_heady = (p->y >> 8) - 5;
	int player_footy = (p->y >> 8) + 5;

	head_data.target_x = player_x;
	head_data.target_y = player_heady;
	foot_data.target_x = player_x;
	foot_data.target_y = player_footy;

	bline(soldier_x, soldier_heady, player_x, player_heady, soldier_eyeline, &head_data);
	bline(soldier_x, soldier_heady, player_x, player_footy, soldier_eyeline, &foot_data);
	if (head_data.seen || foot_data.seen) {
		s->tsd.soldier.last_seen_x = p->x >> 8;
		s->tsd.soldier.last_seen_y = p->y >> 8;
		s->tsd.soldier.sees_player_now = 1;
	} else {
		s->tsd.soldier.sees_player_now = 0;
	}
}

static void maybe_shoot_player(struct gulag_object *s)
{
	int angle;
	int dx, dy;

	if (!s->tsd.soldier.sees_player_now)
		return;
	if (s->tsd.soldier.shoot_cooldown > 0) {
		s->tsd.soldier.shoot_cooldown--;
		return;
	}
	dy = (player.y >> 8) - ((s->y >> 8) + 4);
	dx = (player.x >> 8) - ((s->x >> 8) + 8);
	dx += random_num(10) - 5; /* make them miss sometimes */
	dy += random_num(10) - 5;
	angle = arctan2(dy, -dx);
	if (angle < 0)
		angle += 128;
	fire_bullet(&player, (s->x >> 8) + 4, (s->y >> 8) + 8, angle, s - &go[0]);
	s->tsd.soldier.shoot_cooldown = difficulty[difficulty_level].soldier_shoot_cooldown;
}

static void move_soldier(struct gulag_object *s)
{
	int dx, dy, angle, newx, newy, p;
	struct path_data *pd;
#define SOLDIER_MOVE_THROTTLE 2

	/* only move every nth tick, and move every soldier on a different tick */
	p = s->tsd.soldier.path_index;
	pd = &path_data[p];
	int n = s - &go[0];
	if (((game_timer + n) % SOLDIER_MOVE_THROTTLE) != 0)
		return;
	soldier_look_for_player(s, &player);
	maybe_shoot_player(s);

	switch (s->tsd.soldier.state) {
	case SOLDIER_STATE_RESTING:
		if (random_num(1000) < 900) /* 90% chance he continues to do nothing */
			break;
		/* Or choose a destination and start moving there. */
		do {
			dx = random_num(COST_XDIM);
			dy = random_num(COST_YDIM);
		} while (room_cost[dy][dx] != 0);
		s->tsd.soldier.destx = dx;
		s->tsd.soldier.desty = dy;
		pd->nsteps = 0;
		pd->current_step = 0;
		s->tsd.soldier.state = SOLDIER_STATE_MOVING;
		/* TODO: maybe if we see the player, do something else. */
		break;
	case SOLDIER_STATE_MOVING:
		/* Have we consumed all pathfinding steps? */
		if (pd->current_step == pd->nsteps) {
			/* Have we arrived at our desitination? */
			dx = astarx_to_8dot8x(s->tsd.soldier.destx);
			dy = astary_to_8dot8y(s->tsd.soldier.desty);
			dx = abs(dx - s->x);
			dy = abs(dy - s->y);
			if (dx <= (1 << 8) + (1 << 7) && dy <= (1 << 8) + (1 << 7)) {
				/* We have arrived at our destination */
				s->tsd.soldier.state = SOLDIER_STATE_RESTING;
			} else {
				/* We need more pathfinding steps */
				int sx = fpdot8x_to_astarx(s->x);
				int sy = fpdot8y_to_astary(s->y);
				int gx = s->tsd.soldier.destx;
				int gy = s->tsd.soldier.desty;
				void *start = &room_cost[sy][sx];
				void *goal = &room_cost[gy][gx];
				struct a_star_path *path = a_star(room_cost, &astar_workspace, start, goal,
						ASTAR_MAXNODES, distance_fn, cost_fn, neighbor_fn);
				if (!path) {
					/* We do not expect this to happen. */
					s->tsd.soldier.state = SOLDIER_STATE_RESTING;
					pd->nsteps = 0;
					pd->current_step = 0;
					printf("path finding failed! (%d,%d) to (%d, %d)\n", sx, sy, gx, gy);
					printf("costs are %d, %d\n", room_cost[sy][sx], room_cost[gy][gx]);
					/* FIXME: this will cause a bad cycle where soldier
					 * constantly fails at pathfinding */
					break;
				}
#if 0
				/* Debug code */
				for (int i = 0; i < path->node_count; i++) {
					int x, y;

					astar_node_get_xy(path->path[i], &x, &y);
					printf("xy = %d, %d\n", x, y);
				}
				printf("------\n");
#endif
				/* Copy up to SOLDIER_PATH_LENGTH steps into the soldiers cached pathing data */
				pd->nsteps = path->node_count > SOLDIER_PATH_LENGTH ? SOLDIER_PATH_LENGTH : path->node_count;
				for (int i = 0; i < pd->nsteps; i++) {
					int x, y;

					astar_node_get_xy(path->path[i], &x, &y);
					pd->pathx[i] = x;
					pd->pathy[i] = y;
				}
				pd->current_step = 1; /* We're standing on zero already */
			}
		} else { /* move in the direction of the next pathfinding step */
			dx = astarx_to_8dot8x(pd->pathx[pd->current_step]);
			dy = astary_to_8dot8y(pd->pathy[pd->current_step]);

			dx = (dx >> 8) - (s->x >> 8);
			dy = (dy >> 8) - (s->y >> 8);

			angle = arctan2(dy, -dx);
			if (angle < 0)
				angle += 128;
			s->tsd.soldier.angle = (unsigned char) angle;
			newx = ((-cosine(angle) * difficulty[difficulty_level].soldier_speed) >> 8) + s->x;
			newy = ((sine(angle) * difficulty[difficulty_level].soldier_speed) >> 8) + s->y;

			s->x = newx;
			s->y = newy;
			/* Have we arrived at our next path step? */
			dx = astarx_to_8dot8x(pd->pathx[pd->current_step]);
			dy = astary_to_8dot8y(pd->pathy[pd->current_step]);
			dx = abs(dx - s->x);
			dy = abs(dy - s->y);
			if (dx <= (1 << 8) + (1 << 7) && dy <= (1 << 8) + (1 << 7)) {
				/* Yes, we have arrived at our next path step, advance to the next one. */
				pd->current_step++;
				if (pd->current_step > pd->nsteps) /* paranoia, shouldn't happen */
					pd->current_step = pd->nsteps;
			}
		}
		advance_soldier_animation(s);
		screen_changed = 1;
		break;
	case SOLDIER_STATE_CHASING:
	case SOLDIER_STATE_FLEEING:
	case SOLDIER_STATE_SHOOTING:
	case SOLDIER_STATE_HANDSUP:
	default:
		break;
	}
}

static void explode_grenade(struct grenade *o)
{
	int i;

	for (i = 0; i < GRENADE_FRAGMENTS; i++) {
		int angle = random_num(127);
		fire_bullet(&player, o->x >> 8, o->y >> 8, angle, BULLET_SOURCE_GRENADE);
		screen_changed = 1;
	}
}

static void draw_grenade(struct grenade *o)
{
	int x1 = o->x >> 8;
	int y1 = o->y >> 8;
	int x2 = (o->x >> 8) + 1;
	int y2 = (o->y >> 8) + 1;

	if (x1 < 0 || x2 < 0 || x1 >= LCD_XSIZE || x2 >= LCD_XSIZE)
		return;
	if (y1 < 0 || y2 < 0 || y1 >= LCD_YSIZE || y2 >= LCD_YSIZE)
		return;
	FbColor(WHITE);
	FbMove(x1, y1);
	FbRectangle(2, 2);
}

static void move_grenade(struct grenade *o)
{
	int collision, bbx1, bby1, bbx2, bby2, wall_index[2], nwallindices;
	int nx, ny; /* nx, ny must be int, not short, to prevent overflow */
	nx = o->x + o->vx;
	ny = o->y + o->vy;

	/* Bounce off exterior walls */
	if (nx < 0) {
		nx = 1;
		o->vx = (abs(o->vx) * grenade_speed_reduction) >> 8;
		o->vy = (o->vy * grenade_speed_reduction) >> 8;
	} else if (nx >= (127 << 8)) {
		nx = (126 << 8) - 1;
		o->vx = (-abs(o->vx) * grenade_speed_reduction) >> 8;
		o->vy = (o->vy * grenade_speed_reduction) >> 8;
	}
	if (ny < 0) {
		ny = 1;
		o->vx = (o->vx * grenade_speed_reduction) >> 8;
		o->vy = (abs(o->vy) * grenade_speed_reduction) >> 8;
	} else if (ny >= ((127 - 16) << 8)) {
		ny = ((127 - 16) << 8) - 1;
		o->vx = (o->vx * grenade_speed_reduction) >> 8;
		o->vy = (-abs(o->vy) * grenade_speed_reduction) >> 8;
	}

	/* Bounce off interior walls */
	bbx1 = nx - (1 << 8);
	bby1 = ny - (1 << 8);
	bbx2 = nx + (1 << 8);
	bby2 = ny + (1 << 8);
	nwallindices = 2;
	collision = bbox_interior_wall_collision(&castle, player.room, bbx1, bby1, bbx2, bby2, wall_index, &nwallindices);
	if (collision) {
		int n = castle.room[player.room].interior_walls;
		const int8_t *ws = wall_spec[n];
		for (int j = 0; j < nwallindices; j++) {
			if (wall_spec_x(ws[wall_index[j]]) == wall_spec_x(ws[wall_index[j] + 1])) { /* vertical wall */
				o->vx = (grenade_speed_reduction * -o->vx) >> 8;
				o->vy = (grenade_speed_reduction * o->vy) >> 8;
			}
			if (wall_spec_y(ws[wall_index[j]]) == wall_spec_y(ws[wall_index[j] + 1])) { /* horizontal wall */
				o->vx = (grenade_speed_reduction * o->vx) >> 8;
				o->vy = (grenade_speed_reduction * -o->vy) >> 8;
			}
		}
	} else {
		o->x = nx;
		o->y = ny;
	}

	if (o->fuse > 0) {
		o->fuse--;
		/* Explode when fuse reaches 1, not 0. This is so
		 * that there's one more tick after the explosion
		 * so screen_changed gets set to 1 and the explosion
		 * gets erased. Otherwise the explosion would sit there
		 * until something else moved and changed the screen.
		 */
		if (o->fuse == 1)
			explode_grenade(o);
	}
	screen_changed = 1;
}

static void move_grenades(void)
{
	for (int i = 0; i < ngrenades; i++)
		move_grenade(&grenade[i]);

	/* Do this loop backwards so the memmove() in remove_grenade()
	 * has less work to do.
	 */
	for (int i = ngrenades - 1; i >= 0; i--)
		if (grenade[i].fuse == 0)
			remove_grenade(&grenade[i]);
}

static void move_objects(void)
{
	int room = player.room;
	int n = castle.room[room].nobjs;

	for (int i = 0; i < n; i++) {
		int j = castle.room[room].obj[i];
		struct gulag_object *o = &go[j];
		gulag_object_moving_function move = objconst[o->type].move;
		if (move)
			move(o);
	}
}

static void gulag_run()
{
	check_buttons();
	if (gulag_state != GULAG_RUN)
		return;
	draw_screen();
	move_objects();
	move_grenades();
	maybe_search_for_loot();
}

static void gulag_start_menu()
{
	static char menu_initialized = 0;
	if (!menu_initialized) {
		dynmenu_init(&start_menu, start_menu_item, START_MENU_SIZE);
		strcpy(start_menu.title, "GOODBYE GULAG");
		strcpy(start_menu.title2, "CHOOSE");
		strcpy(start_menu.title3, "DIFICULTY");
		dynmenu_set_colors(&start_menu, BLUE, YELLOW);
		dynmenu_add_item(&start_menu, "EASY", GULAG_RUN, 0);
		dynmenu_add_item(&start_menu, "MEDIUM", GULAG_RUN, 1);
		dynmenu_add_item(&start_menu, "HARD", GULAG_RUN, 2);
		dynmenu_add_item(&start_menu, "INSANE", GULAG_RUN, 3);
		dynmenu_add_item(&start_menu, "QUIT", GULAG_EXIT, 4);
		menu_initialized = 1;
	}
	dynmenu_draw(&start_menu);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	int rotary_switch = button_get_rotation(0);

	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_A, down_latches)) {
		int choice = start_menu.item[start_menu.current_item].cookie;
		if (choice >= 0 && choice < 4) {
			difficulty_level = choice;
			player.start_time_ms = rtc_get_ms_since_boot();
		}
		gulag_state = start_menu.item[start_menu.current_item].next_state;
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary_switch > 0)
		dynmenu_change_current_selection(&start_menu, 1);
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary_switch < 0)
		dynmenu_change_current_selection(&start_menu, -1);
}

static void gulag_maybe_exit()
{
	static char menu_initialized = 0;
	if (!menu_initialized) {
		dynmenu_init(&quit_menu, quit_menu_item, 2);
		strcpy(quit_menu.title, "QUIT NOW?");
		dynmenu_set_colors(&quit_menu, BLUE, YELLOW);
		dynmenu_add_item(&quit_menu, "NO, DON'T QUIT", GULAG_RUN, 0);
		dynmenu_add_item(&quit_menu, "YES, QUIT NOW", GULAG_EXIT, 0);
		menu_initialized = 1;
	}
	dynmenu_draw(&quit_menu);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	int rotary_switch = button_get_rotation(0);

	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_A, down_latches))
		gulag_state = quit_menu.item[quit_menu.current_item].next_state;
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary_switch > 0)
		dynmenu_change_current_selection(&quit_menu, 1);
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary_switch < 0)
		dynmenu_change_current_selection(&quit_menu, -1);
}

static void gulag_exit()
{
	gulag_state = GULAG_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

static void gulag_player_died()
{
	static int state = 0;

	if (state < 255) {
		/* Fade screen to black */
		led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, 255 - state);
		state++;
	} else if (state == 255) {
		char buf[255];
		FbClear();
		FbColor(RED);
		FbMove(10, 10);
		snprintf(buf, sizeof(buf),
			"OUR UKRANIAN\nHERO FOUGHT\nBRAVELY AND\n"
			"KILLED %d\nENEMIES\nBEFORE\nTRAGICALLY\n"
			"BEING KILLED\nIN ACTION", player.kills);
		FbWriteString(buf);
		FbSwapBuffers();
		state++;
	} else if (state > 255) {
		if (state >= 512) {
			state = 0;
			gulag_state = GULAG_INIT;
		} else {
			/* Fade screen brightness up */
			led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, state - 256);
			state++;
		}
	}
}

static void draw_safecracking_screen(struct gulag_object *s)
{
	char buf[4];
	int x, y, r1, r2, r3;
	int angle = s->tsd.safe.angle;

	FbClear();
	FbColor(CYAN);
	FbMove(3, 3);
	FbWriteString("CRACK THE SAFE\n");
	if (player.has_combo[get_room_floor(player.room)])
		FbWriteString(safe_combo[get_room_floor(player.room)]);

	snprintf(buf, sizeof(buf), "%02d", 64 - (angle / 2) - 1);
	FbMove(LCD_XSIZE / 2 - 10, 30);
	FbWriteString(buf);
	x = LCD_XSIZE / 2;
	y = LCD_YSIZE / 2 + 20;
	r2 = ((80 * LCD_XSIZE / 2) / 100);
	r1 = ((40 * LCD_XSIZE / 2) / 100);
	r3 = ((70 * LCD_XSIZE / 2) / 100);

	FbCircle(x, y, r2 + 1);
	FbCircle(x, y, r1);
	r1 = ((75 * LCD_XSIZE / 2) / 100);

	int i, dx1, dy1, dx2, dy2;

	for (i = 0; i < 64; i++) {
		int a = (2 * i) + angle;
		while (a >= 128)
			a = a - 128;
		while (a < 0)
			a = a + 128;
		if ((i & 0x03) == 0) {
			dx1 = (r3 * cosine(a)) >> 8;
			dy1 = (r3 * sine(a)) >> 8;
		} else {
			dx1 = (r1 * cosine(a)) >> 8;
			dy1 = (r1 * sine(a)) >> 8;
		}
		dx2 = (r2 * cosine(a)) >> 8;
		dy2 = (r2 * sine(a)) >> 8;
		if (i == 48)
			FbColor(WHITE);
		else
			FbColor(CYAN);
		FbLine(x + dx1, y + dy1, x + dx2, y + dy2);
	}
	FbSwapBuffers();
}

static void gulag_safecracking()
{
	struct gulag_object *s;
	static int first_number = -1;
	static int second_number = -1;
	static int third_number = -1;
	int angle, i, n, d;

	if (player.current_safe == -1) {
		gulag_state = GULAG_RUN;
		return;
	}
	s = &go[player.current_safe];

	draw_safecracking_screen(s);

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		printf("Trying combo R%d, L%d, R%d\n", first_number, second_number, third_number);
		if (first_number == s->tsd.safe.combo[0] &&
			second_number == s->tsd.safe.combo[1] &&
			third_number == s->tsd.safe.combo[2]) {
			printf("Safe is opened!\n");
			s->tsd.safe.opened = 1;
			player.documents_collected |= (1 << s->tsd.safe.documents);
			gulag_state = GULAG_SAFE_CRACKED;
			return;
		} else {
			printf("Safe is not opened\n");
		}
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_SW2, down_latches)) {
		/* Give up on cracking the safe */
		player.safecracking_cooldown = SAFECRACK_COOLDOWN_TIME;
		gulag_state = GULAG_RUN;
		return;
	}

	int rotary_switch = button_get_rotation(0);
	if (rotary_switch == 0)
		return;

	n = abs(rotary_switch);
	d = (rotary_switch < 0) ? -1 : 1;

	for (i = 0; i < n; i++) {
		angle = s->tsd.safe.angle;
		angle += d;
		while (angle < 0)
			angle += 128;
		while (angle >= 128)
			angle -= 128;

		switch (s->tsd.safe.state) {
		case COMBO_STATE_WAITING_FOR_FIRST_REV:
			first_number = -1;
			second_number = -1;
			third_number = -1;
			if (angle == s->tsd.safe.first_angle && rotary_switch > 0 &&
				s->tsd.safe.angle != s->tsd.safe.first_angle) {
				s->tsd.safe.state = COMBO_STATE_WAITING_FOR_SECOND_REV;
				printf("Waiting for 2nd rev\n");
			}
			break;
		case COMBO_STATE_WAITING_FOR_SECOND_REV:
			if (angle == s->tsd.safe.first_angle && rotary_switch > 0 &&
				s->tsd.safe.angle != s->tsd.safe.first_angle) {
				s->tsd.safe.state = COMBO_STATE_GETTING_FIRST_NUMBER;
				printf("Getting first number\n");
			}
			break;
		case COMBO_STATE_GETTING_FIRST_NUMBER:
			if (rotary_switch < 0) {
				first_number = 64 - (s->tsd.safe.angle / 2) - 1;
				printf("Got first number: %d\n", first_number);
				s->tsd.safe.state = COMBO_STATE_WAITING_FOR_LEFT_REV;
				s->tsd.safe.second_angle = s->tsd.safe.angle;
			}
			break;
		case COMBO_STATE_WAITING_FOR_LEFT_REV:
			if (angle == s->tsd.safe.second_angle && rotary_switch < 0 &&
				s->tsd.safe.angle != s->tsd.safe.second_angle) {
				s->tsd.safe.state = COMBO_STATE_GETTING_SECOND_NUMBER;
				printf("Getting second number\n");
			}
			break;
		case COMBO_STATE_GETTING_SECOND_NUMBER:
			if (rotary_switch > 0) {
				second_number = 64 - (s->tsd.safe.angle / 2) - 1;
				printf("Got second number: %d\n", second_number);
				s->tsd.safe.state = COMBO_STATE_GETTING_THIRD_NUMBER;
			}
			break;
		case COMBO_STATE_GETTING_THIRD_NUMBER:
			third_number = 64 - (angle / 2) - 1;
			printf("third number = %d\n", third_number);
			if (rotary_switch < 0) {
				s->tsd.safe.state = COMBO_STATE_WAITING_FOR_FIRST_REV;
				printf("Waiting for first rev\n");
			}
			break;
		}
	}
	s->tsd.safe.angle = angle;
}

static void gulag_safe_cracked(void)
{
	struct gulag_object *s = &go[player.current_safe];

	FbClear();
	FbColor(CYAN);
	FbMove(3, 3);
	FbWriteString("YOU OPENED THE\nSAFE AND FOUND\nTOP SECRET\n");
	FbWriteString(safe_documents[s->tsd.safe.documents]);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		gulag_state = GULAG_RUN;
}

void gulag_cb(void)
{
	game_timer++;
	switch (gulag_state) {
	case GULAG_INIT:
		gulag_init();
		break;
	case GULAG_INTRO:
		gulag_intro();
		break;
	case GULAG_SCROLL_TEXT:
		gulag_scroll_text();
		break;
	case GULAG_FLAG:
		gulag_flag();
		break;
	case GULAG_START_MENU:
		gulag_start_menu();
		break;
	case GULAG_RUN:
		gulag_run();
		break;
	case GULAG_MAYBE_EXIT:
		gulag_maybe_exit();
		break;
	case GULAG_PLAYER_DIED:
		gulag_player_died();
		break;
	case GULAG_SAFECRACKING:
		gulag_safecracking();
		break;
	case GULAG_SAFE_CRACKED:
		gulag_safe_cracked();
		break;
	case GULAG_PLAYER_WINS:
		gulag_player_wins();
		break;
	case GULAG_PRINT_STATS:
		gulag_print_stats();
		break;
	case GULAG_VIEW_COMBO:
		gulag_view_combo();
		break;
	case GULAG_EXIT:
		gulag_exit();
		break;
	default:
		break;
	}
}

