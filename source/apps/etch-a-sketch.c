#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "accelerometer.h"

#define FGCOLOR x11_black
#define BGCOLOR x11_gray

/* Program states.  Initial state is ETCH_A_SKETCH_INIT */
enum etch_a_sketch_state_t {
	ETCH_A_SKETCH_INIT,
	ETCH_A_SKETCH_RUN,
	ETCH_A_SKETCH_EXIT,
};

static struct game_state {
	int x, y;
	int pen_down;
} state;

static enum etch_a_sketch_state_t etch_a_sketch_state = ETCH_A_SKETCH_INIT;
static int screen_changed = 0;

static void etch_a_sketch_init(void)
{
	FbInit();
	FbColor(FGCOLOR);
	FbBackgroundColor(BGCOLOR);
	FbClear();
	etch_a_sketch_state = ETCH_A_SKETCH_RUN;
	screen_changed = 1;
	state.x = LCD_XSIZE / 2;
	state.y = LCD_YSIZE / 2;
	state.pen_down = 1;
}

static void move(int x, int y)
{
	int nx, ny;

	nx = state.x + x;
	ny = state.y + y;
	if (nx < 0)
		nx = 0;
	if (nx > LCD_XSIZE - 1)
		nx = LCD_XSIZE - 1;
	if (ny < 0)
		ny = 0;
	if (ny > LCD_YSIZE - 1)
		ny = LCD_YSIZE - 1;
	FbColor(state.pen_down ? FGCOLOR : BGCOLOR);
	FbLine(state.x, state.y, nx, ny);
	screen_changed = 1;
	state.x = nx;
	state.y = ny;
}

static void check_buttons(void)
{
	int vrot, hrot;

	vrot = button_get_rotation(0);
	hrot = button_get_rotation(1);
	if (hrot || vrot)
		move(hrot, vrot);

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		etch_a_sketch_state = ETCH_A_SKETCH_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		state.pen_down = !state.pen_down;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		move(-1, 0);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		move(1, 0);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		move(0, -1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		move(0, 1);
	}
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbPushBuffer();
	screen_changed = 0;
}

#define NSAMPLES 5
static union acceleration acc_sample[NSAMPLES];

static void check_accelerometer(void)
{
	static int n = 0;
	union acceleration a;

	union acceleration acceleration = accelerometer_last_sample();

	/* Average the last few samples */
	acc_sample[n] = acceleration;
	n = n + 1;
	if (n >= NSAMPLES)
		n = 0;
	a.x = 0;
	a.y = 0;
	a.z = 0;
	for (int i = 0; i < NSAMPLES; i++) {
		a.x += acc_sample[i].x;
		a.y += acc_sample[i].y;
		a.z += acc_sample[i].z;
	}
	a.x /= NSAMPLES;
	a.y /= NSAMPLES;
	a.z /= NSAMPLES;

	if (a.z < -900) { /* screen facing floor, more or less, erase everything */
		FbColor(BGCOLOR);
		FbClear();
		screen_changed = 1;
	}
}

static void etch_a_sketch_run(void)
{
	check_accelerometer();
	check_buttons();
	draw_screen();
}

static void etch_a_sketch_exit(void)
{
	etch_a_sketch_state = ETCH_A_SKETCH_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void etch_a_sketch_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (etch_a_sketch_state) {
	case ETCH_A_SKETCH_INIT:
		etch_a_sketch_init();
		break;
	case ETCH_A_SKETCH_RUN:
		etch_a_sketch_run();
		break;
	case ETCH_A_SKETCH_EXIT:
		etch_a_sketch_exit();
		break;
	default:
		break;
	}
}

