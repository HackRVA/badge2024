/* dynmenu.h
 *
 * This file defines the interface to a dynamic menu system
 * where the menu items can be updated at run time.
 */

/*

Simple example of how to use this dynmenu thing:

static void my_cool_menu(void)
{
	static int menu_setup = 0;
	static struct dynmenu menu;
	static struct dynmenu_item menu_item[5];

	if (!menu_setup) {
		dynmenu_init(&menu, menu_item, 5);
		dynmenu_clear(&menu);
		dynmenu_set_title(&menu, "MY COOL MENU", "", "");
		dynmenu_add_item(&menu, "COOL CHOICE 1", 0, 1);
		dynmenu_add_item(&menu, "COOL CHOICE 2", 0, 2);
		dynmenu_add_item(&menu, "COOL CHOICE 3", 0, 3);
		menu_setup = 1;
	}

	if (!dynmenu_let_user_choose(&menu))
		return; // let dynmenu take over as runningApp for a bit

	// dynmenu is done, the user has made their choice.
	switch (dynmenu_get_user_choice(&menu)) { // get choice and reset for next time.
	case 1:
		do_cool_thing_1();
		break;
	case 2:
		do_cool_thing_2();
		break;
	case 3:
		do_cool_thing_3();
		break;
	case DYNMENU_SELECTION_ABORTED:
		alright_nevermind_then();
		break;
	}
}

*/

#include <menu.h>

// defines maximum length of a menu item
// 14 chars * 8px per char + 13 * 1px spacing + 1px start + 1px end = 127
// +1 for trailing \0
#define DYNMENU_MAX_TITLE 15

/* Each menu item is defined by an instance of this structure. */
struct dynmenu_item {
	char text[DYNMENU_MAX_TITLE];		/* text to be displayed for this menu item. */
	int next_state;		/* The next program state upon menu selection (see NOTES below) */
	unsigned char cookie;	/* for use by the badge app how it sees fit. */
};

/* This structure defines a menu. */
struct dynmenu {
	char title[DYNMENU_MAX_TITLE]; /* Up to three lines of text to be used as a menu title. */
	char title2[DYNMENU_MAX_TITLE];
	char title3[DYNMENU_MAX_TITLE];
	struct dynmenu_item *item;	/* Up to max_items on the menu */
	unsigned char nitems;		/* Current actual number of items on the menu */
	unsigned char max_items;	/* Maximum possible number of items on the menu. */
	unsigned char current_item;	/* Position within the menu of the selection box. */
	unsigned char menu_active;	/* Is this menu active? currently on screen? */
	unsigned char chosen_cookie;	/* Contains the cookie of the most recently selected item. */
	int color, selected_color;	/* Color of menu text and color of currently selected item. */
#define DYNMENU_SELECTION_VOID (-1)
#define DYNMENU_SELECTION_ABORTED (-2)
	int selection_made;		/* For use by dynmenu_let_user_choose(). */
	void (*original_app)(struct menu_t *menu); /* used by dynmenu_let_user_choose(). */
};

/* dynmenu_init(): Initialize a dynamic menu, dm.
 * @item is an array of struct dynmenu_item that you provide.
 * @max_item_count is the the number of elements @item provides.
 *
 * Typical usage:
 *
 * struct dynmenu menu;
 * struct dynmenu_item item[5];
 * ...
 * dynmenu_init(&menu, item, ARRAYSIZE(item));
 */
void dynmenu_init(struct dynmenu *dm, struct dynmenu_item *item, unsigned char max_item_count);

/* Set the color of the menu text and the color of the selected item */
void dynmenu_set_colors(struct dynmenu *dm, int color, int selected_color);

/* Clears the menu titles and sets ->nitems = 0; */
void dynmenu_clear(struct dynmenu *dm);

/* Set the title(s) of the menu.  If title1, title2, or title3 are NULL, those
 * titles are unchanged.  Use "" to clear a title.
 */
void dynmenu_set_title(struct dynmenu *dm, const char *title1, const char *title2, const char *title3);

/* Adds a new item to the end of the menu. */
void dynmenu_add_item(struct dynmenu *dm, const char *text, int next_state, unsigned char cookie);

/* Draws the menu. */
void dynmenu_draw(struct dynmenu *dm);

/* Adjust the current selection up (direction is negative) or down (direction is positive).
 * Typically, direction is either 1 or -1, and connected to the D-pad buttons.
 */
void dynmenu_change_current_selection(struct dynmenu *dm, int direction);

/* Temporarily take over the runningApp to get a user selection.
 * This is meant to be called only from badge apps.
 * When the user has made a selection, or exited the menu without
 * selecting anything, the badge app will become the running app
 * again, and dm->selection_made will contain one of:
 *
 * 1. DYNMENU_SELECTION_ABORTED - the user didn't make any selection
 * 2. the cookie of the selected item.
 *
 * The badge app should usually use dynmenu_get_user_choice() to
 * read the value of dm->selection_made, as this will automatically
 * reset it to DYNMENU_SELECTION_VOID, to set up for the next time
 * dynmenu_let_user_choose() will be called.
 *
 * Initially, (after dynmenu_init), and after dynmenu_get_user_choice()
 * returns, dm->selection_made will contain DYNMENU_SELECTION_VOID,
 * indicating the user hasn't even been asked to make a selection yet.
 *
 * Note dynmenu_let_user_choose() does NOT return what the user has
 * chosen.  It returns 0 prior to the user having made a choice, and
 * 1 once the user has made a choice.
 */
int dynmenu_let_user_choose(struct dynmenu *dm);

/* Return whatever the user has chosen once dynmenu_let_user_choose has finished */
int dynmenu_get_user_choice(struct dynmenu *dm);

/* NOTES:

The assumption and intent of the "next_state" field of the menu items is that the
badge app has a "program_state" variable and the main callback function of the badge
app is something like this:

enum my_program_state {
	MY_APP_INIT=0,
	WHATEVER,
	BLAH,
	ETC,
	...
};

#define MAX_MENU_ITEMS 5 // or however many you need
static struct dynmenu my_app_menu;
static struct dynmenu_item item[MAX_MENU_ITEMS];

static void do_app_init(void)
{
	dynmenu_init(&my_app_menu, item, ARRAYSIZE(item));
	dynmenu_clear(&my_app_menu);
	dynmenu_add_item(&my_app_menu, "Whatever", WHATEVER, 0);
	dynmenu_add_item(&my_app_menu, "Blah", BLAH, 0);
	dynmenu_add_item(&my_app_menu, "Etc", ETC, 0);
	// and whatever other app init stuff you might need
}

int badge_app_cb(void)
{
	switch (my_app_state) {
	case MY_APP_INIT:
		do_app_init();
		break;
	case WHATEVER:
		do_whatever();
		break;
	case BLAH:
		do_blah();
		break;
	case ETC:
		do_etc();
		break;
	...

	}
}

So, somewhere in those functions, you'll have something like this:

   if (down_latches & (1<<BADGE_BUTTON_ENCODER_SW)) {
        button_pressed();

and then...

static void button_pressed(void)
{
    ...
    if (my_app_menu.menu_active) {
        // change the program state based on menu selection
        my_app_state = my_app_menu.item[my_app_menu.current_item].next_state;
        my_app_menu.chosen_cookie = my_app_menu.item[my_app_menu.current_item].cookie;
        my_app_menu.menu_active = 0;
        return;
    }
    ...
}

*/

