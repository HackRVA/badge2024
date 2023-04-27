#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "trig.h"
#include "fxp_sqrt.h"
#include "xorshift.h"
#include "random.h"
#include "rtc.h"

#if TARGET_PICO
#define printf(...)
#endif

#define DEBUG_MARKERS 0

#define TANK_COLOR x11_green
#define TERRAIN_COLOR x11_dark_green
#define OBSTACLE_COLOR x11_forest_green
#define SPARK_COLOR YELLOW
#define RADAR_COLOR x11_red
#define RADAR_BLIP_COLOR WHITE

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

enum tank_mode {
	TANK_MODE_IDLE,
	TANK_MODE_AVOIDING_OBSTACLE,
	TANK_MODE_DRIVING,
	TANK_MODE_COMPUTE_STEERING, /* figuring which way to turn */
	TANK_MODE_STEERING,		/* turning */
	TANK_MODE_AIMING,
	TANK_MODE_SHOOTING,
	TANK_MODE_SHOOTING_COOLDOWN,
};

static struct tank_brain {
	enum tank_mode mode;
	int dest_x, dest_z;
	int desired_orientation;
	int cooldown;
	int obstacle_timer;
#define TANK_DEST_ARRIVE_DIST (10 << 8)
} tank_brain = { 0 };

struct bz_vertex {
	int32_t x, y, z; /* 3d coord */
	int32_t px, py; /* projected vertex */
};

struct bz_model {
	int nvertices;
	int nsegs;
	struct bz_vertex *vert;
	int16_t *vlist;
	int prescale_numerator, prescale_denominator;
};

struct bz_object {
	int32_t x, y, z;
	int scale;
	int orientation;
	int alive;
	int vx, vy, vz;
	int parent_obj;
#define NO_PARENT_OBJ (-2)
#define PLAYER_PARENT_OBJ (-1)
	uint16_t color;
	unsigned char model;
};

static struct bz_vertex bz_cube_verts[] = {
	{ -10,  20,  10, 0, 0 },
	{  10,  20,  10, 0, 0 },
	{  10,  20, -10, 0, 0 },
	{ -10,  20, -10, 0, 0 },
	{ -10,   0,  10, 0, 0 },
	{  10,   0,  10, 0, 0 },
	{  10,   0, -10, 0, 0 },
	{ -10,   0, -10, 0, 0 },
};

static int16_t bz_cube_vlist[] = {
	0, 1, 2, 3, 0,
	4, 5, 6, 7, 4, -1,
	1, 5, -1,
	2, 6, -1,
	3, 7,
};

static struct bz_vertex bz_short_cube_verts[] = {
	{ -10,  10,  10, 0, 0 },
	{  10,  10,  10, 0, 0 },
	{  10,  10, -10, 0, 0 },
	{ -10,  10, -10, 0, 0 },
	{ -10,   0,  10, 0, 0 },
	{  10,   0,  10, 0, 0 },
	{  10,   0, -10, 0, 0 },
	{ -10,   0, -10, 0, 0 },
};

static int16_t bz_short_cube_vlist[] = {
	0, 1, 2, 3, 0,
	4, 5, 6, 7, 4, -1,
	1, 5, -1,
	2, 6, -1,
	3, 7,
};

static struct bz_vertex bz_pyramid_verts[] = {
	{ -10,   0,  10, 0, 0, },
	{  10,   0,  10, 0, 0, },
	{  10,   0, -10, 0, 0, },
	{ -10,   0, -10, 0, 0, },
	{   0,  20,   0, 0, 0, },
};

static int16_t bz_pyramid_vlist[] = {
	0, 1, 2, 3, 0, 4, 1, -1,
	4, 2, -1,
	4, 3,
};

static struct bz_vertex bz_narrow_pyramid_verts[] = {
	{ -5,   0,  5, 0, 0, },
	{  5,   0,  5, 0, 0, },
	{  5,   0, -5, 0, 0, },
	{ -5,   0, -5, 0, 0, },
	{   0,  20,   0, 0, 0, },
};

static int16_t bz_narrow_pyramid_vlist[] = {
	0, 1, 2, 3, 0, 4, 1, -1,
	4, 2, -1,
	4, 3,
};

static struct bz_vertex bz_horiz_line_verts[] = {
	{ -10, 0, 0, 0, 0 },
	{  10, 0, 0, 0, 0 },
};

static int16_t bz_horiz_line_vlist[] = {
	0, 1,
};

static struct bz_vertex bz_vert_line_verts[] = {
	{ 0, 20, 0, 0, 0 },
	{ 0, 0,  0, 0, 0 },
};

static int16_t bz_vert_line_vlist[] = {
	0, 1,
};

static struct bz_vertex bz_tank_verts[] = {
	/* Bottom */
	{ -50, 0, 100, 0, 0 }, /* 0 */
	{ -50, 0, -100, 0, 0 },
	{  50, 0, -100, 0, 0 },
	{  50, 0, 100, 0, 0 },

	/* Mid section */
	{ -60, 30, 120, 0, 0 }, /* 4 */
	{  -60, 30, -120, 0, 0 },
	{  60, 30, -120,  0, 0 },
	{  60, 30, 120, 0, 0 },

	/* Top */
	{ -50, 50, 80, 0, 0 }, /* 8 */
	{ -50, 50, -50, 0, 0 },
	{  50, 50, -50, 0, 0 },
	{  50, 50, 80, 0, 0 },

	/* Turret top */
	{ -25, 80, 60, 0, 0 }, /* 12 */
	{ -25, 80, 15, 0, 0 },
	{  25, 80, 15, 0, 0 },
	{  25, 80, 60, 0, 0 },

	/* Vertical parts of turret */
	{ -30, 50, 70, 0, 0 }, /* 16 */
	{ -30, 50,   0, 0, 0 },
	{  30, 50,   0, 0, 0 },
	{  30, 50, 70, 0, 0 },

	/* barrel */
	{ 0, 70, 0, 0, 0 }, /* 20 */
	{ 0, 70, -170, 0, 0 },
	{ 5, 65, 0, 0, 0 },
	{ 5, 65, -170, 0, 0 },
	{ -5, 65, 0, 0, 0 },
	{ -5, 65, -170, 0, 0 },
};

static int16_t bz_tank_vlist[] = {
	0, 1, 2, 3, 0,
	4, 5, 6, 7, 4, -1,
	1, 5, -1,
	2, 6, -1,
	3, 7, -1,
	8, 9, 10, 11, 8,
	4, -1,
	9, 5, -1,
	10, 6, -1,
	11, 7, -1,
	12, 13, 14, 15, 12,
	16, -1,
	13, 17, -1,
	14, 18, -1,
	15, 19, -1,
	20, 21, -1,
	22, 23, -1,
	24, 25, -1,
	21, 23, 25,
};

static struct bz_vertex bz_artillery_shell_vert[] = {
	{ 0, 0, 1, 0, 0 },
	{ 0, 1, 0, 0, 0 },
	{ 0, 0, -1, 0, 0 },
	{ 0, -1, 0, 0, 0 },
	{ -1, 0, 0, 0, 0 },
	{ 1, 0, 0, 0, 0 },
};

static int16_t bz_artillery_shell_vlist[] = {
	0, 1, 2, 3, 0, 4, 2, 5, 0, -1,
	1, 4, 3, 5, 1,
};

static struct bz_vertex bz_chunk0_vert[] = {
	{ -3, 1, 2, 0, 0 },
	{  3, 4, 0, 0, 0 },
	{  4, -1, 4, 0, 0 }, 
	{  1, -2, -1, 0, 0 },
};

static int16_t bz_chunk0_vlist[] = {
	0, 1, 2, 0, 3, 2, -1, 3, 1,
};

static struct bz_vertex bz_chunk1_vert[] = {
	{ -3, 3, 0, 0, 0 },
	{  0, -2, 0, 0, 0 },
	{  3, -1, 0, 0, 0 },
};

static int16_t bz_chunk1_vlist[] = {
	0, 1, 2, 0,
};

static struct bz_vertex bz_chunk2_vert[] = {
	{ -4, 2, 0, 0, 0 },
	{  1, -3, 0, 0, 0 },
	{  2, -2, 0, 0, 0 },
};

static int16_t bz_chunk2_vlist[] = {
	0, 1, 2, 0,
};

static struct bz_model bz_cube_model = {
	.nvertices = ARRAYSIZE(bz_cube_verts),
	.nsegs = ARRAYSIZE(bz_cube_vlist),
	.vert = bz_cube_verts,
	.vlist = bz_cube_vlist,
	.prescale_numerator = 1 * 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_short_cube_model = {
	.nvertices = ARRAYSIZE(bz_short_cube_verts),
	.nsegs = ARRAYSIZE(bz_short_cube_vlist),
	.vert = bz_short_cube_verts,
	.vlist = bz_short_cube_vlist,
	.prescale_numerator = 1 * 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_pyramid_model = {
	.nvertices = ARRAYSIZE(bz_pyramid_verts),
	.nsegs = ARRAYSIZE(bz_pyramid_vlist),
	.vert = bz_pyramid_verts,
	.vlist = bz_pyramid_vlist,
	.prescale_numerator = 1 * 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_narrow_pyramid_model = {
	.nvertices = ARRAYSIZE(bz_narrow_pyramid_verts),
	.nsegs = ARRAYSIZE(bz_narrow_pyramid_vlist),
	.vert = bz_narrow_pyramid_verts,
	.vlist = bz_narrow_pyramid_vlist,
	.prescale_numerator = 1 * 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_horiz_line_model = {
	.nvertices = 2,
	.nsegs = 2,
	.vert = bz_horiz_line_verts,
	.vlist = bz_horiz_line_vlist,
	.prescale_numerator = 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_vert_line_model = {
	.nvertices = 2,
	.nsegs = 2,
	.vert = bz_vert_line_verts,
	.vlist = bz_vert_line_vlist,
	.prescale_numerator = 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_tank_model = {
	.nvertices = ARRAYSIZE(bz_tank_verts),
	.nsegs = ARRAYSIZE(bz_tank_vlist),
	.vert = bz_tank_verts,
	.vlist = bz_tank_vlist,
	.prescale_numerator = 256,
	.prescale_denominator = 10,
};

static struct bz_model bz_artillery_shell_model = {
	.nvertices = ARRAYSIZE(bz_artillery_shell_vert),
	.nsegs = ARRAYSIZE(bz_artillery_shell_vlist),
	.vert = bz_artillery_shell_vert,
	.vlist = bz_artillery_shell_vlist,
	.prescale_numerator = 256,
	.prescale_denominator = 4,
};

static struct bz_model bz_chunk0_model = {
	.nvertices = ARRAYSIZE(bz_chunk0_vert),
	.nsegs = ARRAYSIZE(bz_chunk0_vlist),
	.vert = bz_chunk0_vert,
	.vlist = bz_chunk0_vlist,
	.prescale_numerator = 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_chunk1_model = {
	.nvertices = ARRAYSIZE(bz_chunk1_vert),
	.nsegs = ARRAYSIZE(bz_chunk1_vlist),
	.vert = bz_chunk1_vert,
	.vlist = bz_chunk1_vlist,
	.prescale_numerator = 256,
	.prescale_denominator = 1,
};

static struct bz_model bz_chunk2_model = {
	.nvertices = ARRAYSIZE(bz_chunk2_vert),
	.nsegs = ARRAYSIZE(bz_chunk2_vlist),
	.vert = bz_chunk2_vert,
	.vlist = bz_chunk2_vlist,
	.prescale_numerator = 256,
	.prescale_denominator = 1,
};

static const struct bz_model *bz_model[] = {
	&bz_cube_model,
	&bz_short_cube_model,
	&bz_pyramid_model,
	&bz_narrow_pyramid_model,
	&bz_horiz_line_model,
	&bz_vert_line_model,
	&bz_tank_model,
	&bz_artillery_shell_model,
	&bz_chunk0_model,
	&bz_chunk1_model,
	&bz_chunk2_model,
};

static const int nmodels = ARRAYSIZE(bz_model);

#define CUBE_MODEL 0
#define SHORT_CUBE_MODEL 1
#define PYRAMID_MODEL 2
#define NARROW_PYRAMID_MODEL 3
#define HORIZ_LINE_MODEL 4
#define VERT_LINE_MODEL 5
#define TANK_MODEL 6
#define ARTILLERY_SHELL_MODEL 7
#define CHUNK0_MODEL 8
#define CHUNK1_MODEL 9
#define CHUNK2_MODEL 10 

#define MAX_BZ_OBJECTS 100
static struct bz_object bzo[MAX_BZ_OBJECTS] = { 0 };
static int nbz_objects = 0;
static unsigned int xorshift_state = 0;
static int bz_kills = 0;
static int bz_deaths = 0;

/* Approximate replica of the arcade game map */
static const struct bz_map_entry {
	int x, z, type;
} battlezone_map[] = {
	{ 172, 4, SHORT_CUBE_MODEL },
	{ 219, 13, CUBE_MODEL },
	{ 120, 60, NARROW_PYRAMID_MODEL },
	{ 200, 60, PYRAMID_MODEL },
	{ 247, 60, SHORT_CUBE_MODEL },
	{ 39, 76, CUBE_MODEL },
	{ 132, 82, CUBE_MODEL },
	{ 189, 90, NARROW_PYRAMID_MODEL },
	{ 56, 124, SHORT_CUBE_MODEL },
	{ 251, 126, PYRAMID_MODEL },
	{ 54, 135, PYRAMID_MODEL },
	{ 148, 150, NARROW_PYRAMID_MODEL },
	{ 235, 164, CUBE_MODEL },
	{ 56, 181, NARROW_PYRAMID_MODEL },
	{ 95, 188, SHORT_CUBE_MODEL },
	{ 108, 233, SHORT_CUBE_MODEL },
	{ 147, 230, PYRAMID_MODEL },
	{ 57, 253, NARROW_PYRAMID_MODEL },
	{ 120, 253, CUBE_MODEL },
	{ 251, 253, PYRAMID_MODEL },
};

#define CAMERA_GROUND_LEVEL (6 * 256)
static struct camera {
	int32_t x, y, z;
	int orientation;
	int eyedist;
	int vy;
} camera;

#define MAX_SPARKS 100
#define SPARKS_PER_EXPLOSION (MAX_SPARKS / 4)
#define SPARK_GRAVITY (-10)
#define TANK_CHUNK_COUNT (10)
static struct bz_spark {
	int x, y, z, life, vx, vy, vz;
} spark[MAX_SPARKS] = { 0 };
static int nsparks = 0;

static void add_spark(int x, int y, int z, int vx, int vy, int vz, int life)
{
	if (nsparks >= MAX_SPARKS)
		return;
	struct bz_spark *s = &spark[nsparks];
	s->x = x;
	s->y = y;
	s->z = z;
	s->vx = vx;
	s->vy = vy;
	s->vz = vz;
	s->life = life;
	nsparks++;
}

static void remove_spark(int n)
{
	if (n < nsparks - 1)
		spark[n] = spark[nsparks - 1];
	nsparks--;
}

static void move_spark(struct bz_spark *s)
{
	s->x += s->vx;
	s->y += s->vy;
	s->z += s->vz;
	s->vy += SPARK_GRAVITY;
	if (s->y < 0)
		s->life = 0;
	if (s->life > 0)
		s->life--;
}

static void move_sparks(void)
{
	for (int i = 0; i < nsparks; i++)
		move_spark(&spark[i]);
}

static void remove_dead_sparks(void)
{
	for (int i = 0;;) {
		if (i >= nsparks)
			break;
		struct bz_spark *s = &spark[i];
		if (s->life > 0) {
			i++;
			continue;
		}
		remove_spark(i);
	}
}

static signed char mountain[128];

static void fractal_mountain(int start, int middle, int end)
{
	int m;
	if (middle - start > 1) {
		int d = (abs(mountain[middle] - mountain[start]) * 30) / 100;
		if (d > 0)
			m = ((mountain[start] + mountain[middle]) / 2) - (d / 2) + (xorshift(&xorshift_state) % d);
		else
			m = mountain[start];
		int i = (middle - start) / 2 + start;
		mountain[i] = m;
		fractal_mountain(start, i, middle);
	}
	if (end - middle > 1) {
		int d = (abs(mountain[end] - mountain[middle]) * 30) / 100;
		if (d > 0)
			m = ((mountain[middle] + mountain[end]) / 2) - (d / 2) + (xorshift(&xorshift_state) % d);
		else
			m = mountain[middle];
		int i = (end - middle) / 2 + middle;
		mountain[i] = m;
		fractal_mountain(middle, i, end);
	}
}

static void init_mountains(void)
{
	for (int i = 0; i < 128; i++)
		mountain[i] = 80;
	mountain[32] = 20;
	fractal_mountain(0, 32, 96);
}

/* Program states.  Initial state is BATTLEZONE_INIT */
enum battlezone_state_t {
	BATTLEZONE_INIT,
	BATTLEZONE_RUN,
	BATTLEZONE_EXIT,
};

static enum battlezone_state_t battlezone_state = BATTLEZONE_INIT;
static int screen_changed = 0;

static int add_object(int x, int y, int z, int orientation, uint8_t model, uint16_t color)
{
	if (nbz_objects >= MAX_BZ_OBJECTS)
		return -1;
	bzo[nbz_objects].x = x;
	bzo[nbz_objects].y = y;
	bzo[nbz_objects].z = z;
	bzo[nbz_objects].orientation = orientation;
	bzo[nbz_objects].model = model;
	bzo[nbz_objects].color = color;
	bzo[nbz_objects].vx = 0;
	bzo[nbz_objects].vy = 0;
	bzo[nbz_objects].vz = 0;
	bzo[nbz_objects].alive = 1;
	bzo[nbz_objects].parent_obj = NO_PARENT_OBJ;
	nbz_objects++;
	return nbz_objects - 1;
}

static void remove_object(int n)
{
	if (n < nbz_objects - 1)
		bzo[n] = bzo[nbz_objects - 1];
	nbz_objects--;
}

static void prescale_models(void)
{
	static int already_scaled = 0;

	if (already_scaled)
		return;
	already_scaled = 1;

	for (int i = 0; i < nmodels; i++) {
		for (int j = 0; j < bz_model[i]->nvertices; j++) {
			bz_model[i]->vert[j].x *= bz_model[i]->prescale_numerator;
			bz_model[i]->vert[j].y *= bz_model[i]->prescale_numerator;
			bz_model[i]->vert[j].z *= bz_model[i]->prescale_numerator;
			bz_model[i]->vert[j].x /= bz_model[i]->prescale_denominator;
			bz_model[i]->vert[j].y /= bz_model[i]->prescale_denominator;
			bz_model[i]->vert[j].z /= bz_model[i]->prescale_denominator;
		}
	}
}

static void add_initial_objects(void)
{
	for (size_t i = 0; i < ARRAYSIZE(battlezone_map); i++) {
		const struct bz_map_entry *m = &battlezone_map[i];
		add_object((m->x - 128) * 512, 0, (m->z - 128) * 512, 0, m->type, OBSTACLE_COLOR);
	}
	add_object(   0, 0, -100 * 256, 0, TANK_MODEL, TANK_COLOR);
	tank_brain.mode = TANK_MODE_IDLE;
	tank_brain.cooldown = 0;
}

static void battlezone_init(void)
{
	if (xorshift_state == 0) {
		random_insecure_bytes((uint8_t *) &xorshift_state, sizeof(xorshift_state));
		init_mountains();
	}

	nbz_objects = 0;
	nsparks = 0;
	prescale_models();
	add_initial_objects();

	camera.x = 0;
	camera.y = CAMERA_GROUND_LEVEL;
	camera.z = 0;
	camera.vy = 0;
	camera.orientation = 0;
	camera.eyedist = 100 * 256;

	FbInit();
	FbClear();
	battlezone_state = BATTLEZONE_RUN;
	screen_changed = 1;
}

static void bump_player()
{
	camera.y = CAMERA_GROUND_LEVEL + (4 * 256);
}

static int shell_collision(struct bz_object *s)
{
	int dx, dz;
	for (int i = 0; i < nbz_objects; i++) {
		if (s == &bzo[i]) /* can't collide with self */
			continue;

		switch (bzo[i].model) {
		case CHUNK0_MODEL: /* Can't collide with "chunks" */
		case CHUNK1_MODEL:
		case CHUNK2_MODEL:
			continue;
		case TANK_MODEL:
			if (i == s->parent_obj) /* tank can't shoot itself */
				continue;
			break;
		default:
			break;
		}

		dx = s->x - bzo[i].x;
		dz = s->z - bzo[i].z;
		if (dx < 0)
			dx = -dx;
		if (dz < 0)
			dz = -dz;
		 if (dx < (8 << 8) &&  dz < (8 << 8))
			return i + 1;
		}

	if (s->parent_obj == PLAYER_PARENT_OBJ) /* player can't hit themselves */
		return 0;

	/* Check if we hit the player */
	dx = s->x - camera.x;
	dz = s->z - camera.z;
	if (dx < 0)
		dx = -dx;
	if (dz < 0)
		dz = -dz;
	 if (dx < (8 << 8) &&  dz < (8 << 8))
		return -1;
	return 0;
}

static int player_obstacle_collision(int nx, int nz)
{
	for (int i = 0; i < nbz_objects; i++) {
		int dx, dz;

		switch (bzo[i].model) {
		case CHUNK0_MODEL: /* can't collide with "chunks" */
		case CHUNK1_MODEL:
		case CHUNK2_MODEL:
			continue;
		default:
			break;
		}

		dx = nx - bzo[i].x;
		dz = nz - bzo[i].z;
		if (dx < 0)
			dx = -dx;
		if (dz < 0)
			dz = -dz;
		 if (dx < (15 << 8) &&  dz < (15 << 8))
			return 1;
	}
	return 0;
}

static int tank_obstacle_collision(struct bz_object *tank, int nx, int nz)
{
	for (int i = 0; i < nbz_objects; i++) {
		int dx, dz;

		if (&bzo[i] == tank) /* Can't collide with self */
			continue;

		switch (bzo[i].model) {
		case CHUNK0_MODEL: /* can't collide with "chunks" */
		case CHUNK1_MODEL:
		case CHUNK2_MODEL:
			continue;
		case ARTILLERY_SHELL_MODEL:
			continue; /* shell_move will get the collision, if any */
#if DEBUG_MARKERS
		case NARROW_PYRAMID_MODEL:
			if (bzo[i].color == RED)
				continue;
			break;
#endif
		default:
			break;
		}

		dx = nx - bzo[i].x;
		dz = nz - bzo[i].z;
		if (dx < 0)
			dx = -dx;
		if (dz < 0)
			dz = -dz;
		 if (dx < (15 << 8) &&  dz < (15 << 8))
			return 1;
	}
	return 0;
}

static int player_has_been_hit = 0;
static void fire_gun(void)
{

#define SHELL_SPEED 5
#define SHELL_LIFETIME 50
#define IDEAL_TARGET_DIST ((SHELL_SPEED * SHELL_LIFETIME * 180) / 256)
#define TANK_SHOOT_COOLDOWN_TIME_MS (3000)

	int n;

	n = add_object(camera.x, camera.y, camera.z, camera.orientation, ARTILLERY_SHELL_MODEL, x11_orange);
	if (n < 0)
		return;
	bzo[n].alive = SHELL_LIFETIME;
	bzo[n].vx = -SHELL_SPEED * sine(camera.orientation);
	bzo[n].vz = -SHELL_SPEED * cosine(camera.orientation);
	bzo[n].vy = 0;
	bzo[n].parent_obj = PLAYER_PARENT_OBJ;
}

static void check_buttons()
{
	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		battlezone_state = BATTLEZONE_EXIT;
		return;
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		fire_gun();
	}
	if (button_poll(BADGE_BUTTON_LEFT)) {
		camera.orientation--;
		if (camera.orientation < 0)
			camera.orientation = 127;
	}
	if (button_poll(BADGE_BUTTON_RIGHT)) {
		camera.orientation++;
		if (camera.orientation > 127)
			camera.orientation = 0;
	}
	if (button_poll(BADGE_BUTTON_UP)) {
		/* This seems "off", but... works?  Something's screwy about the coord system
		 * I think. */
		int nx, nz;
		nx = camera.x - sine(camera.orientation);
		nz = camera.z - cosine(camera.orientation);
		if (!player_obstacle_collision(nx, nz)) {
			camera.x = nx;
			camera.z = nz;
		} else {
			bump_player();
		}
	}
	if (button_poll(BADGE_BUTTON_DOWN)) {
		/* This seems "off", but... works?  Something's screwy about the coord system
		 * I think. */
		int nx, nz;
		nz = camera.z + cosine(camera.orientation);
		nx = camera.x + sine(camera.orientation);
		if (!player_obstacle_collision(nx, nz)) {
			camera.x = nx;
			camera.z = nz;
		} else {
			bump_player();
		}
	}
}

static void project_vertex(struct camera *c, struct bz_vertex *v, struct bz_object *o)
{
	int32_t x, y, z, a, nx, ny, nz;

	a = o->orientation;
	a = -a;
	if (a < 0)
		a = a + 128;
	if (a >= 128)
		a = a - 128;

	/* Rotate for object orientation */
	nx = ((-v->x * cosine(a)) / 256) - ((v->z * sine(a)) / 256);
	ny = v->y;
	nz = ((v->z * cosine(a)) / 256) - ((v->x * sine(a)) / 256); 
	x = nx;
	y = ny;
	z = nz;

	/* Translate for +object position and -camera position */
	x = x + o->x - c->x;
	y = y + o->y - c->y;
	z = z + o->z - c->z;

	/* Rotate for camera */
	a = 128 - c->orientation;
	if (a > 127)
		a = a - 128;

	nx = ((-x * cosine(a)) / 256) - ((z * sine(a)) / 256);
	ny = y;
	nz = ((z * cosine(a)) / 256) - ((x * sine(a)) / 256); 
	x = nx;
	y = ny;
	z = nz;

	if (z >= 0) {
		v->px = -1;
		v->py = -1;
		return;
	}

	v->px = (c->eyedist * x) / -z;
	v->py = (c->eyedist * y) / -z;
	v->px = v->px + (64 * 256);
	v->py = (160 * 256) - (v->py + (80 * 256));
}

static int onscreen(int x, int y)
{
	if (x < 0 || x >= 128)
		return 0;
	if (y < 0 || y >= 160)
		return 0;
	return 1;
}

static void draw_projected_line(struct bz_vertex *v1, struct bz_vertex *v2)
{
	int x1, y1, x2, y2, onscreen1, onscreen2;

	x1 = v1->px / 256;
	y1 = v1->py / 256;
	x2 = v2->px / 256;
	y2 = v2->py / 256;
	onscreen1 = onscreen(x1, y1);
	onscreen2 = onscreen(x2, y2);
	if (!onscreen1 && !onscreen2)
		return;
	if (!onscreen1 || !onscreen2)
		FbClippedLine(x1, y1, x2, y2);
	else
		FbLine(x1, y1, x2, y2);
}

static void draw_object(struct camera *c, int n)
{
	struct bz_model *m = (struct bz_model *) bz_model[bzo[n].model];
	int v1, v2;

	FbColor(bzo[n].color);

	for (int i = 0; i < m->nvertices; i++)
		project_vertex(c, &m->vert[i], &bzo[n]);

	for (int i = 0; i < m->nsegs - 1;) {
		v1 = m->vlist[i];
		v2 = m->vlist[i + 1];
		if (v2 == -1) {
			i = i + 2;
			continue;
		}
		draw_projected_line(&m->vert[v1], &m->vert[v2]);
		i++;
	}
}

static void draw_mountains(void)
{
	int x1 = 0;
	int y1, x2, y2;
	int j;

	FbColor(TERRAIN_COLOR);
	for (int i = 0; i < 23; i++) {
		j = i + camera.orientation;
		if (j > 127)
			j -= 128;
		y1 = mountain[j];
		j++;
		if (j > 127)
			j -= 128;
		y2 = mountain[j];
		x2 = x1 + 1423;
		FbClippedLine(x1 >> 8, y1, x2 >> 8, y2);
		x1 = x2;
		y1 = y2;
	}
}

static void draw_horizon()
{
	FbColor(TERRAIN_COLOR);
	FbHorizontalLine(0, 80, 128, 80);
}

static int inside_view_frustum(struct camera *c, struct bz_object *o)
{
	int dx, dz;
	signed short sdx, sdz;

	dx = o->x - c->x;
	dz = o->z - c->z;

	if (abs(dx) > 32000 || abs(dz) > 32000) {
		dx = dx >> 8;
		dz = dz >> 8;
	}
	sdx = (signed short) dx;
	sdz = (signed short) dz;
	int a = arctan2(-sdx, -sdz);
	if (a < 0)
		a += 128;
	if (a > 127)
		a -= 128;
	a = a - c->orientation;
	if (a < 0)
		a += 128;
	if (a > 127)
		a -= 128;
	return (a < 18 && a >= 0) || (a > 128 - 18 && a < 128);
}

static void draw_objects(struct camera *c)
{
	for (int i = 0; i < nbz_objects; i++)
		if (inside_view_frustum(c, &bzo[i]))
			draw_object(c, i);
}

static void draw_spark(struct camera *c, struct bz_spark *s)
{
	int x, y, z, nx, ny, nz, a, sx, sy;

	/* Translate for +object position and -camera position */
	x = s->x - c->x;
	y = s->y - c->y;
	z = s->z - c->z;

	/* Rotate for camera */
	a = 128 - c->orientation;
	if (a > 127)
		a = a - 128;

	nx = ((-x * cosine(a)) / 256) - ((z * sine(a)) / 256);
	ny = y;
	nz = ((z * cosine(a)) / 256) - ((x * sine(a)) / 256); 
	x = nx;
	y = ny;
	z = nz;

	if (z >= 0)
		return;

	sx = (c->eyedist * x) / -z;
	sy = (c->eyedist * y) / -z;
	sx = sx + (64 * 256);
	sy = (160 * 256) - (sy + (80 * 256));
	if (onscreen(sx >> 8, sy >> 8))
		FbPoint(sx >> 8, sy >> 8);
}

static void draw_sparks(struct camera *c)
{
	FbColor(SPARK_COLOR);
	for (int i = 0; i < nsparks; i++)
		draw_spark(c, &spark[i]);
}

static void draw_radar(void)
{
	static int radar_angle = 0;
	const int rx = 64;
	const int ry = 10;

	radar_angle++;
	if (radar_angle >= 128)
		radar_angle = 0;
	int x = (cosine(radar_angle) * 10) >> 8;
	int y = (sine(radar_angle) * 10) >> 8;
	FbColor(RADAR_COLOR);
	FbLine(rx, ry, rx + x, ry + y);
	FbVerticalLine(64, 0, 64, 2);
	FbVerticalLine(64, 18, 64, 20);
	FbHorizontalLine(54, 10, 56, 10);
	FbHorizontalLine(72, 10, 74, 10);

	if ((radar_angle & 0x03) == 0x03)
		return; /* Make radar blips blink by not drawing them every few frames */

	for (int i = 0; i < nbz_objects; i++) {
		if (bzo[i].model != TANK_MODEL)
			continue;
		int dx, dz, d, tx, tz;
		dx = (bzo[i].x - camera.x) >> 8;
		dz = (bzo[i].z - camera.z) >> 8;

		d = ((dx * dx >> 8)) + ((dz * dz) >> 8);
		if (d > 200)
			continue;
		/* Rotate for camera */
		int a = 128 - camera.orientation;
		if (a > 127)
			a = a - 128;
		int nx = ((-dx * cosine(a)) / 256) - ((dz * sine(a)) / 256);
		int nz = ((dz * cosine(a)) / 256) - ((dx * sine(a)) / 256);
		tx = (7 * nx) >> 8;
		tz = (7 * nz) >> 8;
		FbColor(RADAR_BLIP_COLOR);
		FbPoint((unsigned char) (rx + tx), (unsigned char) (ry + tz));
	}
}

static void explosion(int x, int y, int z, int count, int chunks)
{

	for (int i = 0; i < count; i++) {
		int vx, vy, vz, life;

		vx = ((int) (xorshift(&xorshift_state) % 600) - 300);
		vy = ((int) (xorshift(&xorshift_state) % 600));
		vz = ((int) (xorshift(&xorshift_state) % 600) - 300);

		life = ((int) (xorshift(&xorshift_state) % 30) + 50);
		add_spark(x, y, z, vx, vy, vz, life); 
	}

	for (int i = 0; i < chunks; i++) {
		int n, vx, vy, vz, life, c;

		vx = ((int) (xorshift(&xorshift_state) % 600) - 300);
		vy = ((int) (xorshift(&xorshift_state) % 600));
		vz = ((int) (xorshift(&xorshift_state) % 600) - 300);
		life = ((int) (xorshift(&xorshift_state) % 30) + 150);
		c = ((int) (xorshift(&xorshift_state) % 3)) + CHUNK0_MODEL;

		n = add_object(x, y, z, 0, c, TANK_COLOR);
		if (n < 0)
			return;
		bzo[n].vx = vx;
		bzo[n].vy = vy;
		bzo[n].vz = vz;
		bzo[n].alive = life;
	}
}

#if DEBUG_MARKERS
static int find_debug_marker(void)
{
	for (int i = 0; i < nbz_objects; i++)
		if (bzo[i].model == NARROW_PYRAMID_MODEL && bzo[i].color == RED)
			return i;
	return 0;
}
#endif

static void tank_mode_idle(__attribute__((unused)) struct bz_object *o)
{
#if DEBUG_MARKERS
	static int debug_marker = -1;
#endif
	int x1, z1, x2, z2, a;
	int dx1, dz1, dx2, dz2;

	/* Maybe we are already close enough? */
	dx1 = (camera.x - o->x);
	dz1 = (camera.z - o->z);
	if ((((dx1 * dx1) / 256) + ((dz1 * dz1) / 256) / 256) < (IDEAL_TARGET_DIST * IDEAL_TARGET_DIST)) {
		tank_brain.mode = TANK_MODE_AIMING;
		return;
	}

	/* Pick two points off either flank of player tank and head for the closest one */
	a = camera.orientation;
	a = a + 32;
	if (a >= 128)
		a -= 128;
	if (a < 0)
		a += 128;
	x1 = camera.x - ((IDEAL_TARGET_DIST * sine(a)));
	z1 = camera.z - ((IDEAL_TARGET_DIST * cosine(a)));
	x2 = camera.x + ((IDEAL_TARGET_DIST * sine(a)));
	z2 = camera.z + ((IDEAL_TARGET_DIST * cosine(a)));
	dx1 = x1 - o->x;
	dz1 = z1 - o->z;
	dx2 = x2 - o->x;
	dz2 = z2 - o->z;
	if (dx1 + dz1 < dx2 + dz2) {  /* pick the closest one by manhattan distance */
		tank_brain.dest_x = x1;
		tank_brain.dest_z = z1;
	} else {
		tank_brain.dest_x = x2;
		tank_brain.dest_z = z2;
	}

	tank_brain.mode = TANK_MODE_COMPUTE_STEERING;

#if DEBUG_MARKERS
	if (debug_marker == -1) {
		debug_marker = add_object(tank_brain.dest_x, 0, tank_brain.dest_z, 0, NARROW_PYRAMID_MODEL, RED);
	} else {
		debug_marker = find_debug_marker();
		if (debug_marker >= 0) {
			bzo[debug_marker].x = tank_brain.dest_x;
			bzo[debug_marker].y = 0;
			bzo[debug_marker].z = tank_brain.dest_z;
		}
	}
#endif
}

static void tank_mode_compute_steering(struct bz_object *o)
{
	int dx, dz;
	signed short sdx, sdz;

	dx = tank_brain.dest_x - o->x;
	dz = tank_brain.dest_z - o->z;

	/* This prevents taking arctan2(0, 0); */
	if (abs(dx) < TANK_DEST_ARRIVE_DIST && abs(dz) < TANK_DEST_ARRIVE_DIST) {
		tank_brain.mode = TANK_MODE_AIMING;
		return;
	}

	if (abs(dx) > 32000 || abs(dz) > 32000) {
		dx = dx >> 8;
		dz = dz >> 8;
	}
	sdx = (signed short) dx;
	sdz = (signed short) dz;
	
	int a = arctan2(-sdx, -sdz);
	if (a < 0)
		a += 128;
	tank_brain.desired_orientation = a;
	tank_brain.mode = TANK_MODE_STEERING;
}

static void tank_mode_steering(struct bz_object *o)
{
	int turning_direction;

	int da = tank_brain.desired_orientation - o->orientation;

	if (da == 0) {
		tank_brain.mode = TANK_MODE_DRIVING;
		return;
	}


	if (da < -64 || (da > 0 && da <= 64))
		turning_direction = 1;
	else
		turning_direction = -1;
	o->orientation += turning_direction;
	if (o->orientation < 0)
		o->orientation += 128;
	if (o->orientation >= 128)
		o->orientation -= 128;
}

static void tank_mode_driving(struct bz_object *o)
{
	static int steering_counter = 0;

	int nx, nz;
	int dx, dz;

	dx = camera.x - o->x;
	dz = camera.z - o->z;

	if ((((dx * dx) / 256) + ((dz * dz) / 256) / 256) < (IDEAL_TARGET_DIST * IDEAL_TARGET_DIST)) {
		tank_brain.mode = TANK_MODE_AIMING;
		return;
	}

	dx = tank_brain.dest_x - o->x;
	dz = tank_brain.dest_z - o->z;

	if (abs(dx) < TANK_DEST_ARRIVE_DIST && abs(dz) < TANK_DEST_ARRIVE_DIST) {
		tank_brain.mode = TANK_MODE_AIMING;
		return;
	}

	nx = o->x - (sine(o->orientation));
	nz = o->z - (cosine(o->orientation));
	if (!tank_obstacle_collision(o, nx, nz)) {
		o->x = nx;
		o->z = nz;
	} else {
		tank_brain.mode = TANK_MODE_AVOIDING_OBSTACLE;
		tank_brain.obstacle_timer = 20;
		return;
	}

	/* When we begin steering from far away, we might miss our destination
	 * if we don't course correct every so often
	 */
	steering_counter++;
	if (steering_counter == 10) {
		steering_counter = 0;
		tank_brain.mode = TANK_MODE_COMPUTE_STEERING;
	}
}

static void tank_mode_avoiding_obstacle(__attribute__((unused)) struct bz_object *o)
{
	/* Move backwards, and turn */
	o->x = o->x + (sine(o->orientation));
	o->z = o->z + (cosine(o->orientation));
	if (tank_brain.obstacle_timer & 0x01) {
		o->orientation++;
		if (o->orientation >= 128)
			o->orientation -= 128;
	}
	if (tank_brain.obstacle_timer > 0)
		tank_brain.obstacle_timer--;
	if (tank_brain.obstacle_timer <= 0) {
		tank_brain.obstacle_timer = 0;
		tank_brain.mode = TANK_MODE_IDLE;
	}
}

static void tank_mode_aiming(__attribute__((unused)) struct bz_object *o)
{
	int dx, dz;
	signed short sdx, sdz;

	dx = camera.x - o->x;
	dz = camera.z - o->z;

	/* This prevents taking arctan2(0, 0); */
	if (abs(dx) < TANK_DEST_ARRIVE_DIST && abs(dz) < TANK_DEST_ARRIVE_DIST) {
		tank_brain.mode = TANK_MODE_IDLE; /* FIXME... what to do here? */
		return;
	}

	if (abs(dx) > 32000 || abs(dz) > 32000) {
		dx = dx >> 8;
		dz = dz >> 8;
	}
	sdx = (signed short) dx;
	sdz = (signed short) dz;
	
	int a = arctan2(-sdx, -sdz);
	if (a < 0)
		a += 128;
	tank_brain.desired_orientation = a;

	int turning_direction;

	int da = tank_brain.desired_orientation - o->orientation;

	if (da == 0) {
		tank_brain.mode = TANK_MODE_SHOOTING;
		return;
	}

	if (da < -64 || (da > 0 && da <= 64))
		turning_direction = 1;
	else
		turning_direction = -1;
	o->orientation += turning_direction;
	if (o->orientation < 0)
		o->orientation += 128;
	if (o->orientation >= 128)
		o->orientation -= 128;
}

static void tank_mode_shooting(struct bz_object *o)
{
	int n;
	n = add_object(o->x, camera.y, o->z, o->orientation, ARTILLERY_SHELL_MODEL, x11_orange);
	if (n < 0) {
		tank_brain.mode = TANK_MODE_SHOOTING_COOLDOWN;
		tank_brain.cooldown = rtc_get_ms_since_boot() + TANK_SHOOT_COOLDOWN_TIME_MS;
		return;
	}
	bzo[n].alive = SHELL_LIFETIME;
	bzo[n].vx = -SHELL_SPEED * sine(o->orientation);
	bzo[n].vz = -SHELL_SPEED * cosine(o->orientation);
	bzo[n].vy = 0;
	bzo[n].parent_obj = o - &bzo[0];
	tank_brain.mode = TANK_MODE_SHOOTING_COOLDOWN;
	tank_brain.cooldown = rtc_get_ms_since_boot() + TANK_SHOOT_COOLDOWN_TIME_MS;
}

static void tank_mode_shooting_cooldown(void)
{
	int n = rtc_get_ms_since_boot();
	if (n > tank_brain.cooldown) {
		tank_brain.cooldown = 0;
		tank_brain.mode = TANK_MODE_IDLE;
	}
}

static void move_tank(struct bz_object *o)
{
	switch (tank_brain.mode) {
	case TANK_MODE_IDLE:
		tank_mode_idle(o);
		break;
	case TANK_MODE_AVOIDING_OBSTACLE:
		tank_mode_avoiding_obstacle(o);
		break;
	case TANK_MODE_DRIVING:
		tank_mode_driving(o);
		break;
	case TANK_MODE_COMPUTE_STEERING:
		tank_mode_compute_steering(o);
		break;
	case TANK_MODE_STEERING:
		tank_mode_steering(o);
		break;
	case TANK_MODE_AIMING:
		tank_mode_aiming(o);
		break;
	case TANK_MODE_SHOOTING:
		tank_mode_shooting(o);
		break;
	case TANK_MODE_SHOOTING_COOLDOWN:
		tank_mode_shooting_cooldown();
		break;
	default:
		tank_brain.mode = TANK_MODE_IDLE;
		break;
	}
}

static void move_object(struct bz_object *o)
{
	int n;

	switch (o->model) {
	case TANK_MODEL:
		move_tank(o);
		break;
	case CHUNK0_MODEL:
	case CHUNK1_MODEL:
	case CHUNK2_MODEL:
		o->x += o->vx;
		o->y += o->vy;
		o->z += o->vz;
		o->vy += SPARK_GRAVITY;
		if (o->alive > 0)
			o->alive--;
		if (o->y < 0)
			o->alive = 0; 
		n = ((o - &bzo[0]) % 6) - 3;
		if (n == 0)
			n = 1;
		o->orientation = o->orientation + n;
		if (o->orientation < 0)
			o->orientation += 128;
		if (o->orientation >= 128)
			o->orientation -= 128;
		break;
	case ARTILLERY_SHELL_MODEL:
		o->x += o->vx;
		o->z += o->vz;
		if (o->alive > 0)
			o->alive--;
		if ((n = shell_collision(o)) != 0) {
			n--; /* shell_collision returns 0 if no collision, object index + 1 if collision */
			if (n < 0) {
				// printf("Hit player!\n");
				o->alive = 0;
				player_has_been_hit = 1;
				explosion(camera.x, camera.y, camera.z, SPARKS_PER_EXPLOSION, 0);
				bz_deaths++;
				break;
			}
			if (bzo[n].model == TANK_MODEL) {
				explosion(o->x, o->y, o->z, SPARKS_PER_EXPLOSION, TANK_CHUNK_COUNT);
				bzo[n].alive = 0;
				bz_kills++;
			} else {
				explosion(o->x, o->y, o->z, SPARKS_PER_EXPLOSION, 0);
			}
			o->alive = 0;
		}
		break;
	default:
		return;
	}
}

static void regenerate_tank(void)
{
	int x, z, orientation;

	x = (((int) xorshift(&xorshift_state)) % 256);
	z = (((int) xorshift(&xorshift_state)) % 256);
	orientation = (((int) xorshift(&xorshift_state)) % 128);
	if (orientation < 0)
		orientation = - orientation;

	add_object((x - 128) * 256, 0, (z - 128) * 256, orientation, TANK_MODEL, TANK_COLOR);
	tank_brain.mode = TANK_MODE_IDLE;
	tank_brain.cooldown = 0;
}

static void move_objects(void)
{
	int tank_count = 0;

	for (int i = 0; i < nbz_objects; i++) {
		move_object(&bzo[i]);
		if (bzo[i].model == TANK_MODEL)
			tank_count++;
	}

	if (tank_count == 0)
		regenerate_tank();

	/* If camera is above normal ground level, make it fall */
	if (camera.y > CAMERA_GROUND_LEVEL) {
		camera.vy -= 5;
		camera.y += camera.vy;
		if (camera.y <= CAMERA_GROUND_LEVEL) {
			camera.y = CAMERA_GROUND_LEVEL;
			camera.vy = 0;
		}
	}
}

static void remove_dead_objects(void)
{
	for (int i = 0;;) {
		if (i >= nbz_objects)
			break;
		struct bz_object *o = &bzo[i];
		if (o->alive > 0) {
			i++;
			continue;
		}
		remove_object(i);
	}
}

static void draw_screen()
{
	char buf[15];

	player_has_been_hit = 0;
	move_objects();
	remove_dead_objects();
	move_sparks();
	remove_dead_sparks();

	if (player_has_been_hit) {
		FbBackgroundColor(WHITE);
		FbClear();
		FbBackgroundColor(BLACK);
		FbSwapBuffers();
		return;
	}

	draw_horizon();
	draw_mountains();
	draw_objects(&camera);
	draw_sparks(&camera);
	draw_radar();
#if 0
	FbColor(WHITE);
	snprintf(buf, sizeof(buf), "%d %d %d", camera.orientation, camera.x / 256, camera.z / 256);	
	FbMove(0, 150);
	FbWriteString(buf);
#endif
	FbColor(GREEN);
	snprintf(buf, sizeof(buf), "%d/%d", bz_kills, bz_deaths);
	FbMove(0, 0);
	FbWriteString(buf);

	FbSwapBuffers();
}

static void battlezone_run()
{
#if REGULATE_FRAMERATE
	static uint64_t last_frame_time = (uint64_t) -1;
	uint64_t diff_time;

	diff_time = rtc_get_ms_since_boot() - last_frame_time;
	if (diff_time >= 33) {
#endif
		check_buttons();
		draw_screen();
#if REGULATE_FRAMERATE
		last_frame_time = rtc_get_ms_since_boot();
	}
#endif
}

static void battlezone_exit()
{
	battlezone_state = BATTLEZONE_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void battlezone_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (battlezone_state) {
	case BATTLEZONE_INIT:
		battlezone_init();
		break;
	case BATTLEZONE_RUN:
		battlezone_run();
		break;
	case BATTLEZONE_EXIT:
		battlezone_exit();
		break;
	default:
		break;
	}
}

