#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "display.h"
#include "screensavers.h"
#include "badge.h"

extern unsigned short popup_time;

typedef void (*ss_func)(void);

static bool saved_screensaver_disabled = 0;

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
	qix,
	hyperspace_screen_saver,
	holly_screensaver,
	nametag_screensaver,
};
static const int num_screen_savers = sizeof(ss) / sizeof(ss[0]);
static int current_screen_saver = -1;

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
	FbBackgroundColor(BLACK);
	FbClear();
	test_screensavers_state = TEST_SCREENSAVERS_RUN;
	current_screen_saver = -1;
	popup_time = 9 * 30;

	/* Disable the actual screensaver while the screensaver test app is running */
	saved_screensaver_disabled = badge_system_data()->screensaver_disabled;
	badge_system_data()->screensaver_disabled = 1;
	screensaver_set_animation_count(0);
}

static void next_screensaver(int direction)
{
	current_screen_saver += direction;
	if (current_screen_saver < -1)
		current_screen_saver = num_screen_savers - 1;
	if (current_screen_saver >= num_screen_savers)
		current_screen_saver = -1; 
	display_reset();
	FbBackgroundColor(BLACK);
	FbClear();
	screensaver_set_animation_count(0);
}

static void check_buttons(void)
{
	int down_latches = button_down_latches();
	int r0 = button_get_rotation(0);
	int r1 = button_get_rotation(1);
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		test_screensavers_state = TEST_SCREENSAVERS_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		next_screensaver(-1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		next_screensaver(1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		next_screensaver(1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		next_screensaver(-1);
	} else if (r0 > 0 || r1 > 0) {
		next_screensaver(1);
	} else if (r0 < 0 || r1 < 0) {
		next_screensaver(-1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		test_screensavers_state = TEST_SCREENSAVERS_EXIT;
	}
}

static void draw_instructions(void)
{
	FbClear();
	FbColor(CYAN);
	FbBackgroundColor(BLACK);
	FbMove(2, 2);
	FbWriteString(
		"\n"
		"USE ROTARY KNOBS\n"
		"  TO SELECT A\n"
		" STATIC SCREEN\n"
		"    SAVER.\n"
		"\n"
		"  PRESS LEFT\n"
		" ROTARY SWITCH\n"
		"   TO EXIT\n");
	FbPushBuffer();
}

static void draw_screen(void)
{
	if (current_screen_saver == -1)
		draw_instructions();
	else
		ss[current_screen_saver]();
}

static void test_screensavers_run(void)
{
	check_buttons();
	draw_screen();
	popup_time--;
	if (popup_time == 0)
		popup_time = 9 * 30;

	/* Prevent badge from going "dormant" so screen will stay lit */
	button_reset_last_input_timestamp();
}

static void test_screensavers_exit(void)
{
	/* So that when we start again, we do not immediately exit */
	test_screensavers_state = TEST_SCREENSAVERS_INIT;
	popup_time = 0;
	/* Restore the original screensaver_disabled value */
	badge_system_data()->screensaver_disabled = saved_screensaver_disabled;
	display_reset(); /* In case the display got messed up (for unknown reasons it happens). */
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

