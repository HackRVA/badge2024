#ifndef DEFAULT_MENU_APP_H__
#define DEFAULT_MENU_APP_H__
#include "badge.h"
#include "menu.h"

struct default_menu_app_context {
	struct menu_t *menu;
	int top_item;
	int current_item;
	int selected_item;
	int screen_changed;
};

void init_default_menu_app_context(struct default_menu_app_context *c, struct menu_t *menu);

extern struct badge_app default_menu_app;

void default_menu_app_cb(struct badge_app *app);

#endif
