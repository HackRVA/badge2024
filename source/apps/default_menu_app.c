#include "default_menu_app.h"
#include "colors.h"
#include "button.h"
#include "framebuffer.h"

void default_menu_app_cb(__attribute__((unused)) void *context);

struct badge_app default_menu_app = {
	.app_func = default_menu_app_cb,
	.app_context = 0,
};

/* Program states.  Initial state is DEFAULT_MENU_APP_INIT */
enum default_menu_app_state_t {
	DEFAULT_MENU_APP_INIT,
	DEFAULT_MENU_APP_RUN,
	DEFAULT_MENU_APP_EXIT,
};

static enum default_menu_app_state_t default_menu_app_state = DEFAULT_MENU_APP_INIT;
static int screen_changed = 0;

static void default_menu_app_init(void)
{
	FbInit();
	FbClear();
	default_menu_app_state = DEFAULT_MENU_APP_RUN;
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
		default_menu_app_state = DEFAULT_MENU_APP_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		default_menu_app_state = DEFAULT_MENU_APP_EXIT;
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

static void default_menu_app_run(void)
{
	check_buttons();
	draw_screen();
}

static void default_menu_app_exit(void)
{
	default_menu_app_state = DEFAULT_MENU_APP_INIT; /* So that when we start again, we do not immediately exit */
	pop_app();
}

void default_menu_app_cb(__attribute__((unused)) void *context)
{
	switch (default_menu_app_state) {
	case DEFAULT_MENU_APP_INIT:
		default_menu_app_init();
		break;
	case DEFAULT_MENU_APP_RUN:
		default_menu_app_run();
		break;
	case DEFAULT_MENU_APP_EXIT:
		default_menu_app_exit();
		break;
	default:
		break;
	}
}

