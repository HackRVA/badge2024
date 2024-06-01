
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "dynmenu.h"
#include <string.h>
#include <stdio.h>

extern void (*runningApp)(struct menu_t *menu);

static struct dynmenu *current_menu = NULL;

void dynmenu_init(struct dynmenu *dm, struct dynmenu_item *item, unsigned char max_items)
{
	dm->item = item;
	dm->max_items = max_items;
	dm->selection_made = DYNMENU_SELECTION_VOID;
	dm->original_app = runningApp; /* save current badge app */
}

void dynmenu_set_colors(struct dynmenu *dm, int color, int selected_color)
{
	dm->color = color;
	dm->selected_color = selected_color;
}

void dynmenu_clear(struct dynmenu *dm)
{
	dm->title[0] = '\0';
	dm->title2[0] = '\0';
	dm->title3[0] = '\0';
	dm->nitems = 0;
	dm->current_item = 0;
	dm->menu_active = 0;
	dm->chosen_cookie = -1;
	dm->selection_made = DYNMENU_SELECTION_VOID;
}

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

void dynmenu_set_title(struct dynmenu *dm, const char *title1,
			const char *title2, const char *title3)
{
	if (title1)
		snprintf(dm->title, DYNMENU_MAX_TITLE, "%s", title1);
	if (title2)
		snprintf(dm->title2, DYNMENU_MAX_TITLE, "%s", title2);
	if (title3)
		snprintf(dm->title3, DYNMENU_MAX_TITLE, "%s", title3);
}

void dynmenu_add_item(struct dynmenu *dm, const char *text, int next_state, unsigned char cookie)
{
    if (dm->nitems >= dm->max_items) {
#ifdef __linux__
        printf("dynmenu_add_item: WARNING: failed to add menu item %d, max_items = %d\n", dm->nitems, dm->max_items);
        printf("menu title: %s\n", dm->title);
#endif
        return;
    }

    const int i = dm->nitems;
    strncpy(dm->item[i].text, text, sizeof(dm->item[i].text) - 1);
    dm->item[i].next_state = next_state;
    dm->item[i].cookie = cookie;
    dm->nitems++;
    dm->chosen_cookie = dm->item[dm->current_item].cookie;
}

void dynmenu_draw(struct dynmenu *dm)
{
	int i, y, first_item, last_item;

    /* choose a starting point that leaves the current item in the center */
	first_item = dm->current_item - 3;
	if (first_item < 0)
		first_item = 0;
	last_item = dm->current_item + 3;
	if (last_item > dm->nitems - 1)
		last_item = dm->nitems - 1;

    /* write menu title (1 to 3 lines) */
	FbClear();
	FbColor(WHITE);
	FbMove(8, 5);
	FbWriteLine(dm->title);
	if (dm->title2[0] != '\0') {
		FbMove(8, 12);
		FbWriteLine(dm->title2);
	}
	if (dm->title3[0] != '\0') {
		FbMove(8, 19);
		FbWriteLine(dm->title3);
	}

    /* get y position for the first item */
	y = LCD_YSIZE / 2 - 10 * (dm->current_item - first_item);
    /* draw each menu item, color the current item green */
	for (i = first_item; i <= last_item; i++) {
		if (i == dm->current_item) {
			if (dm->selected_color != 0) {
				FbColor(dm->selected_color);
			} else {
				FbColor(GREEN);
			}
		} else {
			if (dm->color !=0) {
				FbColor(dm->color);
			} else {
				FbColor(WHITE);
			}
		}
		FbMove(10, y);
		FbWriteLine(dm->item[i].text);
		y += 10;
	}

    /* draw a green rectangle around the current item */
	FbColor(GREEN);
	FbMove(5, LCD_YSIZE / 2 - 2);
	FbRectangle(LCD_XSIZE - 7, 12);
}

/*
 * Change the current item on the menu by going down (positive direction) or
 * up (negative direction). If we would move past the beginning or end of the menu,
 * wrap around to the other end.
 */
void dynmenu_change_current_selection(struct dynmenu *dm, int direction)
{
	int new = dm->current_item + direction;
    /* wrap around in either direction */
	if (new < 0)
		new = dm->nitems - 1;
	else if (new >= dm->nitems)
		new = 0;
	dm->current_item = new;
	dm->chosen_cookie = dm->item[dm->current_item].cookie;
}

/* This is not meant to be called by badge apps.  It is meant to be called by menu.c
 * through the runningApp function pointer after dynmenu_let_user_choose temporarily
 * takes over the running app to get a user selection.
 */
static void dynmenu_cb(__attribute__((unused)) struct menu_t *m)
{
	static int local_screen_changed = 1;

	if (local_screen_changed) {
		dynmenu_draw(current_menu);
		FbSwapBuffers();
		local_screen_changed = 0;
	}
        int down_latches = button_down_latches();
        if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
                dynmenu_change_current_selection(current_menu, -1);
                local_screen_changed = 1;
        } else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
                dynmenu_change_current_selection(current_menu, 1);
                local_screen_changed = 1;
        } else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		current_menu->selection_made = current_menu->chosen_cookie;
		runningApp = current_menu->original_app; /* switch back to badge app */
		local_screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		current_menu->selection_made = DYNMENU_SELECTION_ABORTED;
		runningApp = current_menu->original_app; /* switch back to badge app */
		local_screen_changed = 1;
	}
	/* I thought about calling current_menu->original_app() here to allow
	 * badge apps to still do things while we're showing the menu, but it's too
	 * fraught with problems, pitfalls and complexity. Don't do it.
	 */
}

/* dynmenu_let_user_choose is a bit unusual.  It should only be called from within a
 * badge app. It temporarily makes dynmenu_cb() the runningApp, allowing the user to make
 * a selection from the menu without the badge app having to code the mechanics of drawing
 * the menu or monitoring button presses and changing the current menu selection, etc.
 */
int dynmenu_let_user_choose(struct dynmenu *dm)
{
	if (dm->selection_made != DYNMENU_SELECTION_VOID)
		return 1;
	current_menu = dm;
	current_menu->original_app = runningApp; /* just in case they didn't call dynmenu_init */
	runningApp = dynmenu_cb; /* make menu.c call dynmenu code instead of badge app */
	return 0;
}

int dynmenu_get_user_choice(struct dynmenu *dm)
{
	int rc = dm->selection_made;
	dm->selection_made = DYNMENU_SELECTION_VOID;
	return rc;
}

