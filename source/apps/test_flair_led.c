/*********************************************

Test flair LED

**********************************************/

#include <string.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"


#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

/* Program states.  Initial state is TEST_FLAIR_INIT */
enum test_flair_state_t {
	TEST_FLAIR_INIT,
	TEST_FLAIR_RUN,
	TEST_FLAIR_EXIT,
};

static enum test_flair_state_t test_flair_state = TEST_FLAIR_INIT;

static void test_flair_init(void)
{
	FbInit();
	FbClear();
	FbColor(WHITE);
	FbMove(2, LCD_YSIZE / 2);
	FbWriteString("TESTING\nFLAIR\nLED");
	FbSwapBuffers();
	test_flair_state = TEST_FLAIR_RUN;
}

static void check_buttons()
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		/* Pressing the button exits the program. You probably want to change this. */
		test_flair_state = TEST_FLAIR_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	}
}

struct rgb_color {
	unsigned char r, g, b;
};

static const struct rgb_color rainbow[] = {
	{ 255, 0, 0 },		/* red */
	{ 255, 125, 0 },	/* orange */
	{ 255, 255, 0 },	/* yellow */
	{ 125, 255, 0 },	/* light green */
	{ 0, 255, 0 },		/* green */
	{ 0, 255, 125 },	/* blueish-green */
	{ 0, 255, 255 },	/* cyan */
	{ 0, 125, 255 },	/* light blue */
	{ 0, 0, 255 },		/* blue */
	{ 125, 0, 255 },	/* purple */
	{ 255, 0, 255 },	/* magenta */
	{ 255, 0, 125 },	/* magenta-ish red */
};

static int get_delta(int c, int goal)
{
	int d;
	const int rate = 5;

	if (c > goal) {
		if (c - goal > rate)
			d = -rate;
		else
			d = goal - c;
		return d;
	}
	if (c < goal) {
		if (goal - c > rate)
			d = rate;
		else
			d = goal - c;
		return d;
	}
	return 0;
}

static int limit_255(int x)
{
	if (x < 0)
		return 0;
	if (x > 255)
		return 255;
	return x;
}

static void update_led_color()
{
	static int current_cardinal_color = 0;
	static int r = 255;
	static int g = 0;
	static int b = 0;
	int dr, dg, db;

	int next_cardinal_color = current_cardinal_color + 1;
	if ((size_t) next_cardinal_color >= ARRAYSIZE(rainbow))
		next_cardinal_color = 0;

	dr = get_delta(r, rainbow[next_cardinal_color].r);
	dg = get_delta(g, rainbow[next_cardinal_color].g);
	db = get_delta(b, rainbow[next_cardinal_color].b);

	if (dr == 0 && dg == 0 && db == 0) {
		current_cardinal_color++;
		if ((size_t) current_cardinal_color >= ARRAYSIZE(rainbow))
			current_cardinal_color = 0;
	}

	r = limit_255(r + dr);
	g = limit_255(g + dg);
	b = limit_255(b + db);

	flareled((unsigned char) r, (unsigned char) g, (unsigned char) b);
}

static void test_flair_run()
{
	check_buttons();
	update_led_color();
}

static void test_flair_exit()
{
	test_flair_state = TEST_FLAIR_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

int test_flair_led_cb(void)
{
	switch (test_flair_state) {
	case TEST_FLAIR_INIT:
		test_flair_init();
		break;
	case TEST_FLAIR_RUN:
		test_flair_run();
		break;
	case TEST_FLAIR_EXIT:
		test_flair_exit();
		break;
	default:
		break;
	}
	return 0;
}

