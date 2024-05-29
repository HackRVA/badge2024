#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "utils.h"

const struct point skyline[] = {
	{ -127, 119 },
	{ -62, 123 },
	{ -62, 117 },
	{ -58, 117 },
	{ -57, 105 },
	{ -55, 116 },
	{ -52, 117 },
	{ -49, 115 },
	{ -43, 115 },
	{ -43, 107 },
	{ -39, 101 },
	{ -37, 107 },
	{ -29, 106 },
	{ -30, 98 },
	{ -22, 97 },
	{ -21, 104 },
	{ -14, 103 },
	{ -13, 108 },
	{ -6, 107 },
	{ -4, 118 },
	{ -4, 95 },
	{ 3, 94 },
	{ 4, 119 },
	{ 4, 98 },
	{ 15, 98 },
	{ 14, 105 },
	{ 20, 105 },
	{ 20, 100 },
	{ 27, 101 },
	{ 27, 119 },
	{ 28, 109 },
	{ 29, 118 },
	{ 45, 120 },
	{ 44, 106 },
	{ 49, 110 },
	{ 51, 113 },
	{ 54, 112 },
	{ 54, 109 },
	{ 57, 103 },
	{ 58, 108 },
	{ 58, 122 },
	{ 126, 123 },
	{ -128, -128 },
	{ 33, 119 },
	{ 33, 122 },
	{ -128, -128 },
	{ 40, 120 },
	{ 40, 122 },
	{ -128, -128 },
};


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
	FbDrawObject(skyline, ARRAY_SIZE(skyline), GREEN, 64, 90, 512);
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

