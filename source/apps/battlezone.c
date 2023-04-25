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

#if TARGET_PICO
#define printf(...)
#endif

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

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
	{ -100, 0, 50, 0, 0 }, /* 0 */
	{  100, 0, 50, 0, 0 },
	{  100, 0, -50, 0, 0 },
	{  -100, 0, -50, 0, 0 },

	/* Mid section */
	{ -120, 30, 60, 0, 0 }, /* 4 */
	{  120, 30, 60, 0, 0 },
	{  120, 30, -60, 0, 0 },
	{  -120, 30, -60, 0, 0 },

	/* Top */
	{ -80, 50, 50, 0, 0 }, /* 8 */
	{  50, 50, 50, 0, 0 },
	{  50, 50, -50, 0, 0 },
	{  -80, 50, -50, 0, 0 },

	/* Turret top */
	{ -60, 80, 25, 0, 0 }, /* 12 */
	{  -15, 80, 25, 0, 0 },
	{  -15, 80, -25, 0, 0 },
	{  -60, 80, -25, 0, 0 },

	/* Vertical parts of turret */
	{ -70, 50, 30, 0, 0 }, /* 16 */
	{  0, 50, 30, 0, 0 },
	{  0, 50, -30, 0, 0 },
	{  -70, 50, -30, 0, 0 },

	/* barrel */
	{ 0, 60, 0, 0, 0 }, /* 20 */
	{ 100, 70, 0, 0, 0 },
	{ 0, 55, -5, 0, 0 },
	{ 100, 65, -5, 0, 0 },
	{ 0, 55, 5, 0, 0 },
	{ 100, 65, 5, 0, 0 },
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
		add_object((m->x - 128) * 512, 0, (m->z - 128) * 512, 0, m->type, x11_dark_green);
	}
	add_object(   0, 0, -100 * 256, 0, TANK_MODEL, GREEN);
	add_object(   100 * 256, 0, -100 * 256, 0, CHUNK0_MODEL, RED);
}

static void battlezone_init(void)
{
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

static void fire_gun(void)
{

#define SHELL_SPEED 5
#define SHELL_LIFETIME 50

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
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		fire_gun();
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		camera.orientation--;
		if (camera.orientation < 0)
			camera.orientation = 127;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		camera.orientation++;
		if (camera.orientation > 127)
			camera.orientation = 0;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
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
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
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
	if (y < 0 || y >= 159)
		return 0;
	return 1;
}

static void draw_projected_line(struct bz_vertex *v1, struct bz_vertex *v2)
{
	int x1, y1, x2, y2;

	x1 = v1->px / 256;
	y1 = v1->py / 256;
	x2 = v2->px / 256;
	y2 = v2->py / 256;
	if (!onscreen(x1, y1) || !onscreen(x2, y2))
		return;
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

static void draw_horizon()
{
	FbColor(x11_dark_green);
	FbHorizontalLine(0, 80, 128, 80);
}

static void draw_objects(struct camera *c)
{
	for (int i = 0; i < nbz_objects; i++)
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
	FbColor(YELLOW);
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
	FbColor(x11_dark_red);
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
		FbColor(WHITE);
		FbPoint((unsigned char) (rx + tx), (unsigned char) (ry + tz));
	}
}

static void explosion(int x, int y, int z, int count, int chunks)
{
	static unsigned int state = 0xa5a5a5a5;

	for (int i = 0; i < count; i++) {
		int vx, vy, vz, life;

		vx = ((int) (xorshift(&state) % 600) - 300);
		vy = ((int) (xorshift(&state) % 600));
		vz = ((int) (xorshift(&state) % 600) - 300);

		life = ((int) (xorshift(&state) % 30) + 50);
		add_spark(x, y, z, vx, vy, vz, life); 
	}

	for (int i = 0; i < chunks; i++) {
		int n, vx, vy, vz, life, c;

		vx = ((int) (xorshift(&state) % 600) - 300);
		vy = ((int) (xorshift(&state) % 600));
		vz = ((int) (xorshift(&state) % 600) - 300);
		life = ((int) (xorshift(&state) % 30) + 150);
		c = ((int) (xorshift(&state) % 3)) + CHUNK0_MODEL;

		n = add_object(x, y, z, 0, c, GREEN); 
		if (n < 0)
			return;
		bzo[n].vx = vx;
		bzo[n].vy = vy;
		bzo[n].vz = vz;
		bzo[n].alive = life;
	}
}

static void move_object(struct bz_object *o)
{
	int n;

	switch (o->model) {
	case TANK_MODEL:
		o->orientation++;
		if (o->orientation >= 128)
			o->orientation = 0;
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
				break;
			}
			if (bzo[n].model == TANK_MODEL) {
				explosion(o->x, o->y, o->z, SPARKS_PER_EXPLOSION, TANK_CHUNK_COUNT);
				bzo[n].alive = 0;
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

static void move_objects(void)
{
	for (int i = 0; i < nbz_objects; i++)
		move_object(&bzo[i]);

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

	move_objects();
	remove_dead_objects();
	move_sparks();
	remove_dead_sparks();

	draw_horizon();
	draw_objects(&camera);
	draw_sparks(&camera);
	draw_radar();
	FbColor(WHITE);
	snprintf(buf, sizeof(buf), "%d %d %d", camera.orientation, camera.x / 256, camera.z / 256);	
	FbMove(0, 150);
	FbWriteString(buf);
	FbSwapBuffers();
}

static void battlezone_run()
{
	check_buttons();
	draw_screen();
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

