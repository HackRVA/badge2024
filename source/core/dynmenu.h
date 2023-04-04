/* dynmenu.h
 *
 * This file defines the interface to a dynamic menu system
 * where the menu items can be updated at run time.
 */

/* Each menu item is defined by an instance of this structure. */
struct dynmenu_item {
	char text[15];		/* text to be displayed for this menu item. */
	int next_state;		/* The next program state upon menu selection (see NOTES below) */
	unsigned char cookie;	/* for use by the badge app how it sees fit. */
};

/* This structure defines a menu. */
struct dynmenu {
	char title[15]; /* Up to three lines of text to be used as a menu title. */
	char title2[15];
	char title3[15];
	struct dynmenu_item *item;	/* Up to max_items on the menu */
	unsigned char nitems;		/* Current actual number of items on the menu */
	unsigned char max_items;	/* Maximum possible number of items on the menu. */
	unsigned char current_item;	/* Position within the menu of the selection box. */
	unsigned char menu_active;	/* Is this menu active? currently on screen? */
	unsigned char chosen_cookie;	/* Contains the cookie of the most recently selected item. */
	int color, selected_color;	/* Color of menu text and color of currently selected item. */
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

/* Adds a new item to the end of the menu. */
void dynmenu_add_item(struct dynmenu *dm, char *text, int next_state, unsigned char cookie);

/* Draws the menu. */
void dynmenu_draw(struct dynmenu *dm);

/* Adjust the current selection up (direction is negative) or down (direction is positive).
 * Typically, direction is either 1 or -1, and connected to the D-pad buttons.
 */
void dynmenu_change_current_selection(struct dynmenu *dm, int direction);

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

