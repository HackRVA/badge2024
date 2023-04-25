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

static struct bz_model bz_cube_model = {
	.nvertices = ARRAYSIZE(bz_cube_verts),
	.nsegs = ARRAYSIZE(bz_cube_vlist),
	.vert = bz_cube_verts,
	.vlist = bz_cube_vlist,
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

static const struct bz_model *bz_model[] = {
	&bz_cube_model,
	&bz_pyramid_model,
	&bz_horiz_line_model,
	&bz_vert_line_model,
	&bz_tank_model,
};

static const int nmodels = ARRAYSIZE(bz_model);

#define CUBE_MODEL 0
#define PYRAMID_MODEL 1
#define HORIZ_LINE_MODEL 2
#define VERT_LINE_MODEL 3
#define TANK_MODEL 4

#define MAX_BZ_OBJECTS 100
static struct bz_object bzo[MAX_BZ_OBJECTS] = { 0 };
static int nbz_objects = 0;

static struct camera {
	int32_t x, y, z;
	int orientation;
	int eyedist;
} camera;

/* Program states.  Initial state is BATTLEZONE_INIT */
enum battlezone_state_t {
	BATTLEZONE_INIT,
	BATTLEZONE_RUN,
	BATTLEZONE_EXIT,
};

static enum battlezone_state_t battlezone_state = BATTLEZONE_INIT;
static int screen_changed = 0;

static void add_object(int x, int y, int z, int orientation, uint8_t model, uint16_t color)
{
	if (nbz_objects >= MAX_BZ_OBJECTS)
		return;
	bzo[nbz_objects].x = x;
	bzo[nbz_objects].y = y;
	bzo[nbz_objects].z = z;
	bzo[nbz_objects].orientation = orientation;
	bzo[nbz_objects].model = model;
	bzo[nbz_objects].color = color;
	nbz_objects++;
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
	add_object( 100 * 256, 0,    0, 0, CUBE_MODEL, GREEN);
	add_object(-100 * 256, 0,    0, 0, PYRAMID_MODEL, CYAN);
	add_object(   0, 0,  100 * 256, 0, CUBE_MODEL, WHITE);
	add_object(   0, 0, -100 * 256, 0, TANK_MODEL, GREEN);
}

static void battlezone_init(void)
{
	prescale_models();
	add_initial_objects();

	camera.x = 0;
	camera.y = 6 * 256;
	camera.z = 0;
	camera.orientation = 0;
	camera.eyedist = 100 * 256;

	FbInit();
	FbClear();
	battlezone_state = BATTLEZONE_RUN;
	screen_changed = 1;
}

static void check_buttons()
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		battlezone_state = BATTLEZONE_EXIT;
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
		camera.z -= cosine(camera.orientation);
		camera.x -= sine(camera.orientation);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		/* This seems "off", but... works?  Something's screwy about the coord system
		 * I think. */
		camera.z += cosine(camera.orientation);
		camera.x += sine(camera.orientation);
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

static void move_object(struct bz_object *o)
{
	if (o->model != TANK_MODEL)
		return;
	o->orientation++;
	if (o->orientation >= 128)
		o->orientation = 0;
}

static void move_objects(void)
{
	for (int i = 0; i < nbz_objects; i++)
		move_object(&bzo[i]);
}

static void draw_screen()
{
	char buf[15];
	draw_horizon();
	move_objects();
	draw_objects(&camera);
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

