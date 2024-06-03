#include <stdio.h>

#include "default_menu_app.h"
#include "colors.h"
#include "button.h"
#include "framebuffer.h"

#define DEFAULT_MENU_FG_COLOR WHITE
#define DEFAULT_MENU_BG_COLOR BLACK

struct default_menu_app_context context_stack[MAX_APP_STACK_DEPTH];
int current_menu_stack_idx = -1;
struct default_menu_app_context *current_context = NULL;

void init_default_menu_app_context(struct default_menu_app_context *c, struct menu_t *m)
{
	c->menu = m;
	c->top_item = 0;
	c->current_item = 0;
	c->selected_item = -1;
	c->screen_changed = 1;
}

void default_menu_app_cb(struct badge_app *app);

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

static void default_menu_app_init(void)
{
	FbInit();
	FbClear();
	default_menu_app_state = DEFAULT_MENU_APP_RUN;
	current_context->screen_changed = 1;
}

static int count_menu_items(struct menu_t *m)
{
	for (int i = 0; ; i++) {
		if (m[i].attrib & LAST_ITEM)
			return i + 1;
	}
}

static void move_up(void)
{
	if (current_context->current_item > 0) {
		current_context->current_item--;
		if (current_context->top_item > current_context->current_item)
			current_context->top_item--;
		current_context->screen_changed = 1;
	}
}

static void move_down(void)
{
	int nitems = count_menu_items(current_context->menu);
	if (current_context->current_item < nitems - 1) {
		current_context->current_item++;
		if (current_context->top_item < current_context->current_item - 15)
			current_context->top_item++;
		current_context->screen_changed = 1;
	}
}

static void go_back(void)
{
	if (current_menu_stack_idx > -1) {
		current_menu_stack_idx--;
		pop_app();
		if (current_menu_stack_idx >= 0)
			current_context = &context_stack[current_menu_stack_idx];
		else
			current_context = default_menu_app.app_context;
	}
}

static void do_selection(void)
{
	struct menu_t *m = current_context->menu;
	enum menu_item_type t = m[current_context->current_item].type;
	struct badge_app app;

	switch (t) {
	case MENU:
		if (current_menu_stack_idx < MAX_APP_STACK_DEPTH - 1) {
			struct badge_app app;

			current_menu_stack_idx++;
			app.app_func = default_menu_app_cb;
			app.wake_up = 1;
			app.app_context = &context_stack[current_menu_stack_idx];
			init_default_menu_app_context(app.app_context,
				(struct menu_t *) &m[current_context->current_item].data.menu[0]);
			push_app(app);
		}
		break;
	case BACK:
		go_back();
		break;
	case FUNCTION:
		app.app_func = m[current_context->current_item].data.func;
		app.app_context = NULL;
		app.wake_up = 1;
		push_app(app);
		break;
	}
}

static void check_buttons(void)
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		move_up();
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		move_down();
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		do_selection();
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		go_back();
	}
	/* skip items tagged to be skipped */
	while (current_context->menu[current_context->current_item].attrib & SKIP_ITEM)
		move_down();
}

static void draw_screen(void)
{
	struct menu_t *m = current_context->menu;

	if (!current_context->screen_changed)
		return;

	FbColor(DEFAULT_MENU_FG_COLOR);
	FbBackgroundColor(DEFAULT_MENU_BG_COLOR);
	FbClear();

	int nitems = count_menu_items(current_context->menu);

	int x = 0;
	int y = 0;
	for (int i = 0; i < nitems; i++) {
		if (i < current_context->top_item) /* skip to the top item */
			continue;
		FbMove(x, y);
		if (i == current_context->current_item) {
			FbColor(DEFAULT_MENU_FG_COLOR);
			FbFilledRectangle(LCD_XSIZE, 10);
			FbColor(DEFAULT_MENU_BG_COLOR);
			FbBackgroundColor(DEFAULT_MENU_FG_COLOR);
		} else {
			FbColor(DEFAULT_MENU_BG_COLOR);
			FbFilledRectangle(LCD_XSIZE, 10);
			FbColor(DEFAULT_MENU_FG_COLOR);
			FbBackgroundColor(DEFAULT_MENU_BG_COLOR);
		}
		FbMove(x, y);
		FbWriteString(m[i].name);
		if (m[i].attrib & LAST_ITEM)
			break;
		y += 10;
		if (y > 150)
			break;
	}
	FbSwapBuffers();
	current_context->screen_changed = 0;
}

static void default_menu_app_run(void)
{
	check_buttons();
	draw_screen();
}

static void default_menu_app_exit(void)
{
	default_menu_app_state = DEFAULT_MENU_APP_INIT; /* So that when we start again, we do not immediately exit */
	(void) pop_app();
}

void default_menu_app_cb(struct badge_app *app)
{
	current_context = app->app_context;
	if (app->wake_up)
		current_context->screen_changed = 1;

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

