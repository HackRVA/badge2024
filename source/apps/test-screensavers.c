#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "screensavers.h"

extern unsigned short popup_time;

typedef void (*ss_func)(void);

static const ss_func ss[] = {
	just_the_badge_tips,
	dotty,
	disp_asset_saver,
	hack_the_dragon,
	stupid_rects,
	carzy_tunnel_animator,
	for_president,
	smiley,
	matrix,
	bluescreen,
};
static const int num_screen_savers = sizeof(ss) / sizeof(ss[0]);
static int current_screen_saver = 0;

/* Program states.  Initial state is TEST_SCREENSAVERS_INIT */
enum test_screensavers_state_t {
	TEST_SCREENSAVERS_INIT,
	TEST_SCREENSAVERS_RUN,
	TEST_SCREENSAVERS_EXIT,
};

static enum test_screensavers_state_t test_screensavers_state = TEST_SCREENSAVERS_INIT;

static void test_screensavers_init(void)
{
	FbInit();
	FbClear();
	test_screensavers_state = TEST_SCREENSAVERS_RUN;
	current_screen_saver = 0;
	popup_time = 9 * 30;
}

static void check_buttons()
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		/* Pressing the button exits the program. You probably want to change this. */
		test_screensavers_state = TEST_SCREENSAVERS_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		current_screen_saver--;
		if (current_screen_saver < 0)
			current_screen_saver = num_screen_savers - 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		current_screen_saver++;
		if (current_screen_saver >= num_screen_savers)
			current_screen_saver = 0;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		current_screen_saver++;
		if (current_screen_saver >= num_screen_savers)
			current_screen_saver = 0;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		current_screen_saver--;
		if (current_screen_saver < 0)
			current_screen_saver = num_screen_savers - 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		test_screensavers_state = TEST_SCREENSAVERS_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		test_screensavers_state = TEST_SCREENSAVERS_EXIT;
	}
}

static void draw_screen()
{
	ss[current_screen_saver]();
}

static void test_screensavers_run()
{
	check_buttons();
	draw_screen();
	popup_time--;
	if (popup_time == 0)
		popup_time = 9 * 30;
}

static void test_screensavers_exit()
{
	/* So that when we start again, we do not immediately exit */
	test_screensavers_state = TEST_SCREENSAVERS_INIT;
	popup_time = 0;
	returnToMenus();
}

void test_screensavers_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (test_screensavers_state) {
	case TEST_SCREENSAVERS_INIT:
		test_screensavers_init();
		break;
	case TEST_SCREENSAVERS_RUN:
		test_screensavers_run();
		break;
	case TEST_SCREENSAVERS_EXIT:
		test_screensavers_exit();
		break;
	default:
		break;
	}
}

