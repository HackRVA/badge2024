#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "display.h"
#include "badge.h"
#include "screensavers.h"
#include "screensaver_app.h"
#include "xorshift.h"
#include "utils.h"

typedef void (*ss_func)(void);

static const ss_func ss[] = {
	just_the_badge_tips,
	dotty,
	disp_asset_saver,
	matrix,
	qix,
	hyperspace_screen_saver,
	nametag_screensaver,
};

static int current_screen_saver = -1;

/* Program states.  Initial state is SCREENSAVER_INIT */
enum screensaver_state_t {
	SCREENSAVER_INIT,
	SCREENSAVER_RUN,
	SCREENSAVER_EXIT,
};

static enum screensaver_state_t screensaver_state = SCREENSAVER_INIT;

static void screensaver_init(void)
{
	static unsigned int state = 0xa5a5a5a5;

	FbInit();
	FbBackgroundColor(BLACK);
	FbClear();
	screensaver_state = SCREENSAVER_RUN;
	current_screen_saver = xorshift(&state) % ARRAY_SIZE(ss);
	screensaver_set_animation_count(0);
}

static void check_buttons(void)
{
	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		screensaver_state = SCREENSAVER_EXIT;
	}
}

static void draw_screen(void)
{
	ss[current_screen_saver]();
}

static void screensaver_run(void)
{
	check_buttons();
	draw_screen();
}

static void screensaver_exit(void)
{
	/* So that when we start again, we do not immediately exit */
	screensaver_state = SCREENSAVER_INIT;
	display_reset(); /* In case the display got messed up (for unknown reasons it happens). */
	pop_app();
}

void screensaver_cb(__attribute__((unused)) struct badge_app *app)
{
	switch (screensaver_state) {
	case SCREENSAVER_INIT:
		screensaver_init();
		break;
	case SCREENSAVER_RUN:
		screensaver_run();
		break;
	case SCREENSAVER_EXIT:
		screensaver_exit();
		break;
	default:
		break;
	}
}

