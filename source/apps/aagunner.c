#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"


/* Program states.  Initial state is AAGUNNER_INIT */
enum aagunner_state_t {
	AAGUNNER_INIT,
	AAGUNNER_RUN,
	AAGUNNER_EXIT,
};

static enum aagunner_state_t aagunner_state = AAGUNNER_INIT;
static int screen_changed = 0;

static void aagunner_init(void)
{
	FbInit();
	FbClear();
	aagunner_state = AAGUNNER_RUN;
	screen_changed = 1;
}

static void check_buttons(void)
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		aagunner_state = AAGUNNER_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		aagunner_state = AAGUNNER_EXIT;
	}
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	FbMove(10, LCD_YSIZE / 2);
	FbWriteLine("HOWDY!");
	FbSwapBuffers();
	screen_changed = 0;
}

static void aagunner_run(void)
{
	check_buttons();
	draw_screen();
}

static void aagunner_exit(void)
{
	aagunner_state = AAGUNNER_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void aagunner_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (aagunner_state) {
	case AAGUNNER_INIT:
		aagunner_init();
		break;
	case AAGUNNER_RUN:
		aagunner_run();
		break;
	case AAGUNNER_EXIT:
		aagunner_exit();
		break;
	default:
		break;
	}
}

