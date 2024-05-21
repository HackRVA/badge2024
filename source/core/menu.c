/*
   simple menu system
   Author: Paul Bruggeman
   paul@Killercats.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TARGET_SIMULATOR
#include <unistd.h>
#include <signal.h> /* for raise() */
#endif
#include "menu.h"
#include "settings.h"
#include "colors.h"
#include "ir.h"
#include "assetList.h"
#include "button.h"
#include "framebuffer.h"
#include "display.h"
#include "audio.h"
#include "led_pwm.h"
#include "music.h"
#include "menu_icon.h"
#include "stacktrace.h"
#include "key_value_storage.h"

// Apps
#include "about_badge.h"
#include "new_badge_monsters/new_badge_monsters.h"
#include "battlezone.h"
#include "game_of_life.h"
#include "hacking_simulator.h"
#include "lunarlander.h"
// #include "pong.h"
#include "qc.h"
#include "smashout.h"
#include "username.h"
#include "slot_machine.h"
// #include "gulag.h"
#include "asteroids.h"
// #include "etch-a-sketch.h"
// #include "magic-8-ball.h"
#include "rvasec_splash.h"
#include "test-screensavers.h"
// #include "tank-vs-tank.h"
#include "clue.h"
#include "moon-patrol.h"
#include "badgey.h"
#include "badge-app-template.h"

/* BUILD_IMAGE_TEST_PROGRAM is defined (or not) in top level CMakelists.txt */
#ifdef BUILD_IMAGE_TEST_PROGRAM
#include "image-test.h"
#endif

#define MAIN_MENU_BKG_COLOR GREY2


extern const struct menu_t schedule_m[]; /* defined in core/schedule.c */

/* offsets for menu animation direction depending on where we came from */
static struct point menu_animation_direction[] = {
    { -1, 0 }, /* came from MENU_PREVIOUS, animation moves left */
    { 1, 0 },  /* came from MENU_NEXT, animation moves right */
    { 0, -1 }, /* came from MENU_PARENT, animation moves up */
    { 0, 1 },  /* came from MENU_CHILD, animation moves down */
    { 0, 0 },  /* came from MENU_UNKNOWN, no movement */
};

static const struct menu_t legacy_games_m[] = {
   {"Battlezone", VERT_ITEM|DEFAULT_ITEM, FUNCTION, { .func = battlezone_cb }, &battlezone_icon, },
   {"Asteroids", VERT_ITEM, FUNCTION, { .func = asteroids_cb }, &asteroids_icon, },
   // {"Etch-a-Sketch", VERT_ITEM, FUNCTION, { .func = etch_a_sketch_cb }, NULL, },
   // {"Magic-8-Ball",     VERT_ITEM, FUNCTION, { .func = magic_8_ball_cb }, NULL, },
   // {"Goodbye Gulag", VERT_ITEM, FUNCTION, { .func = gulag_cb }, NULL, },
   // {"Pong", VERT_ITEM, FUNCTION, { .func = pong_cb }, NULL, },
   // {"Tank vs Tank", VERT_ITEM, FUNCTION, { .func = tank_vs_tank_cb }, NULL, },
   {"Lunar Rescue",  VERT_ITEM, FUNCTION, { .func = lunarlander_cb}, &lunar_rescue_icon, },
   {"Badge Monsters",VERT_ITEM, FUNCTION, { .func = badge_monsters_cb }, &badge_monsters_icon, },
   {"Smashout",      VERT_ITEM, FUNCTION, { .func = smashout_cb }, &breakout_icon, },
   {"Hacking Sim",   VERT_ITEM, FUNCTION, { .func = hacking_simulator_cb }, &hacker_sim_icon, },
   {"Game of Life", VERT_ITEM, FUNCTION, { .func = game_of_life_cb }, &game_of_life_icon, },
   {"Slot Machine", VERT_ITEM, FUNCTION, { .func = slot_machine_cb }, &slotmachine_icon, },
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, { NULL }, NULL, },
};

static const struct menu_t games_m[] = {
	{"Sample App", VERT_ITEM, FUNCTION, { .func = myprogram_cb }, NULL },
	{"Clue", VERT_ITEM|DEFAULT_ITEM, FUNCTION, { .func = clue_cb }, &clue_icon },
	{"Moon Patrol", VERT_ITEM, FUNCTION, { .func = moonpatrol_cb }, &moonpatrol_icon, },
	{"Badgey", VERT_ITEM, FUNCTION, { .func = badgey_cb }, &bba_icon },
	{"Legacy Games",       VERT_ITEM, MENU, { .menu = legacy_games_m }, &legacy_games_icon, },
#ifdef BUILD_IMAGE_TEST_PROGRAM
	{"Image Test", VERT_ITEM, FUNCTION, { .func = image_test_cb }, NULL },
#endif
	{"Back",         VERT_ITEM|LAST_ITEM, BACK, { NULL }, NULL, },
};

static const struct menu_t menu_style_menu_m[] = {
	{"New Menus", VERT_ITEM|DEFAULT_ITEM, FUNCTION, { .func = select_new_menu_style }, NULL, },
	{"Legacy Menus", VERT_ITEM, FUNCTION, { .func = select_legacy_menu_style }, NULL, },
	{"Back", VERT_ITEM|LAST_ITEM, BACK, { NULL }, NULL, },
};

static const struct menu_t menu_speed_m[] = {
	{"Fast Menu", VERT_ITEM|DEFAULT_ITEM, FUNCTION, { .func = select_menu_speed_fast }, NULL, },
	{"Medium Menu", VERT_ITEM, FUNCTION, { .func = select_menu_speed_medium }, NULL, },
	{"Slow Menu", VERT_ITEM, FUNCTION, { .func = select_menu_speed_slow }, NULL, },
	{"Back", VERT_ITEM|LAST_ITEM, BACK, { NULL }, NULL, },
};

static const struct menu_t settings_m[] = {
   {"Backlight", VERT_ITEM, MENU, { .menu = backlight_m }, &backlight_icon, },
   {"LED", VERT_ITEM, MENU, { .menu = LEDlight_m }, &led_icon, },
   {"Audio", VERT_ITEM|DEFAULT_ITEM, MENU, { .menu = buzzer_m }, &audio_icon, },
   {"Invert Display", VERT_ITEM, MENU, { .menu = rotate_m, }, &invert_display_icon, },
   {"User Name", VERT_ITEM, FUNCTION, { .func = username_cb }, &username_icon, },
   {"Screensaver", VERT_ITEM, MENU, { .menu = screen_lock_m }, &screensaver_icon, },
   {"ID", VERT_ITEM, MENU, { .menu = myBadgeid_m }, &id_icon, },
   {"QC",  VERT_ITEM, FUNCTION, { .func = QC_cb }, &qc_icon, },
   {"Clear NVRAM", VERT_ITEM, FUNCTION, { .func = clear_nvram_cb }, &clear_nvram_icon, },
   {"Menu Style", VERT_ITEM, MENU, { .menu = menu_style_menu_m }, &menu_style_icon, },
   {"Menu Speed", VERT_ITEM, MENU, { .menu = menu_speed_m }, &menu_speed_icon, },
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, {NULL}, NULL, },
};

static const struct menu_t main_m[] = {
   {"Schedule",    VERT_ITEM, MENU, { .menu = schedule_m }, &schedule_icon, },
   {"Games",       VERT_ITEM|DEFAULT_ITEM, MENU, { .menu = games_m }, &games_icon, },
   {"Settings",    VERT_ITEM, MENU, { .menu = settings_m }, &settings_icon, },
   {"Test SS",	VERT_ITEM, FUNCTION, { .func = test_screensavers_cb }, NULL, },
   {"About Badge",    VERT_ITEM|LAST_ITEM, FUNCTION, { .func = about_badge_cb }, &about_icon, },
};

/* hack for badge.c to trigger redraw of menu on transition from dormant -> not dormant */
unsigned char menu_redraw_main_menu = 0;

/* Frequency in Hz of beeps to make for various menu actions */
/* Use low pentatonic scale to make it not quite so annoying. */
#define MORE_FREQ NOTE_C3
#define BACK_FREQ NOTE_D3
#define TEXT_FREQ NOTE_E3
#define MENU_FREQ NOTE_G3
#define FUNC_FREQ NOTE_A3
/* Duration of beeps for menu actions, in milliseconds */
#define NOTEDUR 50

static void menu_beep(uint16_t freq)
{
	audio_out_beep(freq, NOTEDUR);
}

#define QC

#ifdef QC
#  define INITIAL_BADGE_APP QC_cb
#else
#  define INITIAL_BADGE_APP rvasec_splash_cb;
#endif
void (*runningApp)(struct menu_t *menu) = INITIAL_BADGE_APP;

#define MORE_INC 4

struct menuStack_t {
   struct menu_t *selectedMenu;
   struct menu_t *currMenu;
   int menu_scroll_start_item;
};

#define MAX_MENU_DEPTH 8
static unsigned char G_menuCnt=0; // index for G_menuStack

static struct menuStack_t G_menuStack[MAX_MENU_DEPTH] = { {0, 0, 0} }; // track user traversing menus

static struct menu_t *G_selectedMenu = NULL; /* item the cursor is on */
static struct menu_t *G_currMenu = NULL; /* init */


struct menu_t *getSelectedMenu() {
    return G_selectedMenu;
}

struct menu_t *getCurrMenu() {
    return G_currMenu;
}

#if 0
/* Nothing uses this */
struct menu_t *getMenuStack(unsigned char item) {
   if (item > G_menuCnt) return 0;

   return G_menuStack[G_menuCnt-item].currMenu;
}
#endif

/* This is only used by settings.c to modify menu item names in the LCD backlight menu. */
struct menu_t *getSelectedMenuStack(unsigned char item) {
   if (item > G_menuCnt) return 0;

   return G_menuStack[G_menuCnt-item].selectedMenu;
}

/*
  currently the char routine draws Y in decreasing (up),
  so 1st Y position has to offset down CHAR_HEIGHT to account for that
*/

unsigned char menu_left=5;

/* these should all be variables or part of a theme */
#define MENU_LEFT menu_left
#define CHAR_WIDTH 9
#define CHAR_HEIGHT 8
#define SCAN_BLANK 1 /* blank lines between text entries */
#define TEXTRECT_OFFSET 1 /* text offset within rectangle */

#define RGBPACKED(R,G,B) ( ((unsigned short)(R)<<11) | ((unsigned short)(G)<<6) | (unsigned short)(B) )

static int menu_scroll_start_item = 0;
#define MENU_MAX_ITEMS_DISPLAYABLE 14 /* how many menu items fit on screen */

/* Upon moving the menu selection, scroll to make selection visible if necessary */
static void maybe_scroll_to(struct menu_t *selected, struct menu_t *current_menu)
{
	int position;

	if (selected == NULL) {
		position = 0;
	} else {
		position = selected - current_menu;
	}
	if (position < menu_scroll_start_item)
		menu_scroll_start_item = position;
	if (position > menu_scroll_start_item + MENU_MAX_ITEMS_DISPLAYABLE - 1)
		menu_scroll_start_item = position - MENU_MAX_ITEMS_DISPLAYABLE + 1;
}

#ifdef TARGET_SIMULATOR
/* This function detects this situation in which we get into an infinite loop
   when searching for the "next" or "previous" item in a menu, as might happen
   if all the menu items were "skip" items.  This loop detection code only exists
   in the simulator.
 */
void detect_infinite_loop_in_menus(struct menu_t *menu, struct menu_t *current_item, int reset)
{
	static int counter = 0;

	if (menu->attrib & LAST_ITEM) {
		stacktrace("Infinite loop detected in menu (first item == last item).\n");
		raise(SIGTRAP); /* break into debugger, or exit */
	}
	if (reset == 1) {
		counter = 0;
		return;
	}

	if (current_item == menu)
		counter++;
	if (counter > 1) {
		stacktrace("Detected infinite loop in menu.\n");
		raise(SIGTRAP); /* break into debugger, or exit */
	}
}
#else
#define detect_infinite_loop_in_menus(menu, current, reset)
#endif

/* Find the next menu item on menu from current_item, skipping items with excluded
   attributes (SKIP_ITEM, typically) optionally skipping "BACK" items,
   and wrapping around to the beginning of the menu as necessary. */
struct menu_t *find_next_menu_item(struct menu_t *menu, struct menu_t *current_item,
		int exclude_attribs, int skip_back_item)
{
	detect_infinite_loop_in_menus(menu, current_item, 1);

	/* If at the last item, we need to wrap around to first item */
	if (current_item->attrib & LAST_ITEM) {
		current_item = menu; /* wrap around to first menu item, */
		if ((current_item->type != BACK || !skip_back_item) &&
			(!(current_item->attrib & exclude_attribs)))
			return current_item;
	}
        while (1) {
		detect_infinite_loop_in_menus(menu, current_item, 0);

		/* Advance to next item in menu, wrapping around if need be */
		current_item++;
		if (current_item->attrib & LAST_ITEM) { /* maybe wrap around to beginning */
			if ((current_item->type == BACK && skip_back_item) ||
				current_item->attrib & exclude_attribs) {
				current_item = menu;
			} else {
				return current_item;
			}
		}
		detect_infinite_loop_in_menus(menu, current_item, 0);
		if (current_item->attrib & exclude_attribs)
			continue;
		if (current_item->type == BACK && skip_back_item)
			continue;
		break;
	}
	return current_item;
}

/* Find the previous menu item on menu from current_item, skipping items with
   excluded attributes (SKIP_ITEM, typically) optionally skipping
   "BACK" items, and wrapping around to the end of the menu as necessary. */
struct menu_t *find_prev_menu_item(struct menu_t *menu, struct menu_t *current_item,
					int exclude_attribs, int skip_back_item)
{
	detect_infinite_loop_in_menus(menu, current_item, 1);
	if (current_item == menu) { /* current is first item, need to return last item */
		while (!(current_item->attrib & LAST_ITEM))
			current_item++;
		if ((!skip_back_item || current_item->type != BACK) && 
			!(current_item->attrib & exclude_attribs))
			return current_item;
		/* else, keep looking backwards ... */
	}
	while (1) {
		/* Advance to previous item... */
		if (current_item == menu) { /* wrap around to last item */
			while (!(current_item->attrib & LAST_ITEM))
				current_item++;
		} else { 
			current_item--;
		}
		detect_infinite_loop_in_menus(menu, current_item, 0);

		if (current_item->type == BACK && skip_back_item)
			continue;
		if (current_item->attrib & exclude_attribs)
			continue;
		break;
	}
	return current_item;
}

#ifdef TARGET_SIMULATOR
static void sanity_check_menu(struct menu_t *menu)
{
	int i;
	int last_found = 0;
	int non_skipped_found = 0;
	struct menu_t *orig = menu;

	for (i = 0; i < 100; i++) {
		if (menu->attrib & LAST_ITEM) {
			last_found = 1;
			break;
		}
		if (!(menu->attrib & SKIP_ITEM))
			non_skipped_found = 1;
		menu++;
	}
	if (!last_found || !non_skipped_found) {
		if (!last_found)
			fprintf(stderr, "menu '%s' has no LAST_ITEM\n", orig->name);
		if (!non_skipped_found)
			fprintf(stderr, "menu '%s' has only SKIP_ITEM items\n", orig->name);
		raise(SIGTRAP);
	}
	if (i > 100) { /* unlikely we'll get here without crashing first */
		fprintf(stderr, "menu '%s' has suspiciously large number of entries\n", orig->name);
		raise(SIGTRAP);
	}
}
#else
#define sanity_check_menu(x)
#endif

static int menu_animation_in_progress = 0;
static int menu_animation_speed = -1;
static struct menu_animation_state {
	int frame;
	enum menu_previous came_from;
	int npoints, old_npoints;
	struct point const *points, *old_points;
	struct menu_t *root_menu;
	struct menu_t *selected;
	int text_label_x;
	int suppress_animation; /* used when exiting apps */
} menu_animation;

#define DEBUG_SUPPRESS_ANIMATION 1
#if DEBUG_SUPPRESS_ANIMATION
#define SUPPRESS_ANIMATION(v, msg) do { \
		menu_animation.suppress_animation = v; \
		printf("animation:%d %s %s:%d\n", v, msg, __FILE__, __LINE__); \
	} while (0)
#else
#define SUPPRESS_ANIMATION(v, msg) do { menu_animation.suppress_animation = v; } while (0)
#endif

/* The reason that legacy_display_menu returns a menu_t * instead of void
   as you might expect is because sometimes it skips over unselectable
   items.

   This is always called through the function pointer display_menu.
 */
static struct menu_t *legacy_display_menu(struct menu_t *menu,
                            struct menu_t *selected,
                            MENU_STYLE style,
			    __attribute__((unused)) enum menu_previous came_from)
{
    static unsigned char cursor_x, cursor_y;
    unsigned char c;
    struct menu_t *root_menu; /* keep a copy in case menu has a bad structure */
    int menu_item_number = 0;

    menu_animation_in_progress = 0; /* legacy menus don't animate */
    root_menu = menu;

    sanity_check_menu(menu);

    switch (style) {
        case MAIN_MENU_STYLE:
            FbBackgroundColor(MAIN_MENU_BKG_COLOR);
            FbClear();

            FbColor(GREEN);
            FbMove(2,5);
            FbRectangle(LCD_XSIZE - 5, LCD_YSIZE - 8);

            FbColor(CYAN);
            FbMove(1,4);
            FbRectangle(LCD_XSIZE - 3, LCD_YSIZE - 6);
            break;

        case WHITE_ON_BLACK:
            FbClear();
            FbBackgroundColor(BLACK);
            FbTransparentIndex(0);
            break;

        case BLANK:
        default:
            break;
    }

    cursor_x = MENU_LEFT;
    //cursor_y = CHAR_HEIGHT;
    cursor_y = 2; // CHAR_HEIGHT;
    FbMove(cursor_x, cursor_y);

    while (1) {
        unsigned char rect_w=0;

	/* Skip menu items until we get to the part of the menu we've scrolled to */
	if (menu_item_number < menu_scroll_start_item) {
		menu++;
		menu_item_number++;
		continue;
	}

        for (c=0, rect_w=0; (menu->name[c] != 0); c++) {
		rect_w += CHAR_WIDTH;
#ifdef TARGET_SIMULATOR
		/* Catch overly long menu names */
		if (c >= sizeof(menu->name) - 1) {
			printf("menu name too long: '%s'\n", menu->name);
			abort();
		}
#endif
	}

        if (menu->attrib & VERT_ITEM) {
            cursor_y += (CHAR_HEIGHT + 2 * SCAN_BLANK);
        }

        if (!(menu->attrib & HORIZ_ITEM)) {
            cursor_x = MENU_LEFT;
        }

        if (selected == menu) {
            // If we happen to be on a skip ITEM, just increment off it
            // The menus() method mostly avoids this, except for some cases
            if (menu->attrib & SKIP_ITEM) selected++;
        }

        if (selected == NULL) {
            if (menu->attrib & DEFAULT_ITEM)
            selected = menu;
        }

        // Determine selected item color
        switch(style) {
            case MAIN_MENU_STYLE:
                if (menu == selected) {
                    FbColor(YELLOW);

                    FbMove(3, cursor_y + 1);
                    FbFilledRectangle(2, 8);

                    // Set the selected color for the coming writeline
                    FbColor(GREEN);
                } else {
                    // unselected writeline color
                    FbColor(GREY16);
                }
                break;
            case WHITE_ON_BLACK:
                FbColor((menu == selected) ? GREEN : WHITE);
                break;
            case BLANK:
            default:
                break;
        }

        FbMove(cursor_x+1, cursor_y+1);
        FbWriteLine(menu->name);
        cursor_x += (rect_w + CHAR_WIDTH);
        if (menu->attrib & LAST_ITEM) break;
        menu++;
	menu_item_number++;
	if (menu_item_number - menu_scroll_start_item >= MENU_MAX_ITEMS_DISPLAYABLE)
		break;
    } // END WHILE

    /* in case last menu item is a skip */
    if (selected == NULL) {
        selected = root_menu;
    }

    // Write menu onto the screen
    FbPushBuffer();
    return selected;
}

static struct point default_menu_drawing[] = { /* just a simple square */
	{ -40, -40, },
	{ 40, -40, },
	{ 40, 40, },
	{ -40, 40, },
	{ -40, -40, },
};

static int menu_has_icons(struct menu_t *m)
{
    /* Check all the menu items and see if any have icons defined.  If not, just use the old menu style */
    int has_icons = 0;
    while (1) {
        if (m->icon != NULL) {
            has_icons = 1;
            break;
        }
        if (m->attrib & LAST_ITEM)
		break;
	m++;
   }
   return has_icons;
}

static int get_menu_animation_speed(void)
{
	if (menu_animation_speed > 0)
		return menu_animation_speed;
	if (!flash_kv_get_int("MENU_SPEED", &menu_animation_speed))
		menu_animation_speed = 20;
	switch (menu_animation_speed) {
	case 10:
	case 20:
	case 40:
		break;
	default:
		menu_animation_speed = 20;
		break;
	}
	return menu_animation_speed;
}

static void animate_menu(struct menu_animation_state *animation)
{
	enum menu_previous came_from = animation->came_from;
	int xd = menu_animation_direction[came_from].x;
	int yd = menu_animation_direction[came_from].y;
	int npoints = animation->npoints;
	struct point const *points = animation->points;
	int old_npoints = animation->old_npoints;
	struct point const *old_points = animation->old_points;
	struct menu_t *root_menu = animation->root_menu;
	struct menu_t *selected = animation->selected;

	FbClear();

	/* Draw new selection entering */
	// int drawing_x = 120 + ((255 - menu_animation.frame) * 100 / 255 - 56);
	int startx = 64 + 56 * -xd; /* 8, or 120 */
	int xoffset = xd * 56 * animation->frame / 255; /* slides from +/- 56 to zero */
	int starty = 64 + 56 * -yd;
	int yoffset = yd * 56 * animation->frame / 255;
	int drawing_x = startx + xoffset;
	int drawing_y = starty + yoffset;
	int drawing_scale = (1024 * (animation->frame + 255) / 2) / 255 / 2;
	FbDrawObject(points, npoints, GREEN, drawing_x, drawing_y, drawing_scale);

	if (came_from != MENU_UNKNOWN) {
		/* Draw previous selection leaving */
		// drawing_x = 64 - (64 * animation->frame / 255);
		startx = 64;
		xoffset = xd * 56 * animation->frame / 255; /* slides from 0 to +/- 56 */
		starty = 64;
		yoffset = yd * 56 * animation->frame / 255;
		drawing_x = startx + xoffset;
		drawing_y = starty + yoffset;
		drawing_scale = (1024 * (255 - animation->frame / 2)) / 255 / 2;
		if (xd != 0 ||
			animation->frame + get_menu_animation_speed() < 255) /* don't draw last frame vertical neighbors */
			FbDrawObject(old_points, old_npoints, GREEN, drawing_x, drawing_y, drawing_scale);
	}

	animation->frame += get_menu_animation_speed();
	if (animation->frame > 255)
		animation->frame = 255;

	if (menu_animation.frame < 255) {
		/* Save menu animation animation state for next frame */
		animation->came_from = came_from;
		animation->npoints = npoints;
		animation->points = points;
		animation->old_npoints = old_npoints;
		animation->old_points = old_points;
		animation->root_menu = root_menu;
		menu_animation_in_progress = 1;
		FbPushBuffer();
		return;
	} else {
		root_menu = animation->root_menu;
		menu_animation_in_progress = 0; /* we've finished the animation */
		animation->frame = 0;
	}
	/* End of menu animation code */

	if (came_from == MENU_PREVIOUS) {
		/* draw the "next" menu item incoming */
		struct menu_t *next_item =
			find_next_menu_item(root_menu, selected, SKIP_ITEM, 1);
		if (next_item && next_item->icon) {
			int drawing_x = 64 + 56;
			int drawing_y = 64;
			int drawing_scale = (1024 * (255 - 250 / 2)) / 255 / 2;
			points = next_item->icon->points;
			int npoints = next_item->icon->npoints;
			FbDrawObject(points, npoints, GREEN, drawing_x, drawing_y, drawing_scale);
		}
	}

	if (came_from == MENU_NEXT) {
		/* draw the "next" menu item incoming */
		struct menu_t *prev_item =
			find_prev_menu_item(root_menu, selected, SKIP_ITEM, 1);
		if (prev_item && prev_item->icon) {
			int drawing_x = 64 - 56;
			int drawing_y = 64;
			int drawing_scale = (1024 * (255 - 250 / 2)) / 255 / 2;
			points = prev_item->icon->points;
			int npoints = prev_item->icon->npoints;
			FbDrawObject(points, npoints, GREEN, drawing_x, drawing_y, drawing_scale);
		}
	}
	FbMove(animation->text_label_x, 120);
	FbWriteLine(selected->name);
	FbPushBuffer();
}

/* The reason that new_display_menu returns a menu_t * instead of void
   as you might expect is because sometimes it skips over unselectable
   items.

   This is always called through the function pointer display_menu.
 */
static struct menu_t *new_display_menu(struct menu_t *menu,
                            struct menu_t *selected,
                            MENU_STYLE style,
			    enum menu_previous came_from)
{
    static unsigned char cursor_x, cursor_y;
    unsigned char c;
    struct menu_t *root_menu; /* keep a copy in case menu has a bad structure */
    int menu_item_number = 0;
    static struct menu_t *last_menu_selected = NULL;
    static struct menu_icon *previous_icon = NULL;

    sanity_check_menu(menu);

    /* If this menu has no icons defined, just use the old legacy style to display
     * This makes, e.g. the schedule continue to work.
     */
    if (!menu_has_icons(menu)) {
       menu_animation_in_progress = 0;
       return legacy_display_menu(menu, selected, style, came_from);
    }

    if (last_menu_selected != selected) {
	last_menu_selected = selected;
	menu_animation.frame = 0;
	menu_animation_in_progress = 1;
    }

    root_menu = menu;

    while (1) {
        unsigned char rect_w=0;

	/* Skip menu items until we get to the part of the menu we've scrolled to */
	if (menu_item_number < menu_scroll_start_item) {
		menu++;
		menu_item_number++;
		continue;
	}

        for (c=0, rect_w=0; (menu->name[c] != 0); c++) {
		rect_w += CHAR_WIDTH;
#ifdef TARGET_SIMULATOR
		/* Catch overly long menu names */
		if (c >= sizeof(menu->name) - 1) {
			printf("menu name too long: '%s'\n", menu->name);
			abort();
		}
#endif
	}

        if (menu->attrib & VERT_ITEM) {
            cursor_y += (CHAR_HEIGHT + 2 * SCAN_BLANK);
        }

        if (!(menu->attrib & HORIZ_ITEM)) {
            cursor_x = MENU_LEFT;
        }

        if (selected == menu) {
            // If we happen to be on a skip ITEM, just increment off it
            // The menus() method mostly avoids this, except for some cases
            if (menu->attrib & SKIP_ITEM) selected++;
        }

        if (selected == NULL) {
            if (menu->attrib & DEFAULT_ITEM)
		    selected = menu;
        }

	if (selected == menu) { /* wtf does this mean? */
		int x = 64 - (strlen(menu->name) / 2) * 8;
		/* Draw new selection item arriving */
		int npoints, old_npoints;
		struct point const *points, *old_points;

		if (menu->icon) {
			npoints = menu->icon->npoints;
			points = menu->icon->points;
		} else {
			npoints = 5;
			points = default_menu_drawing;
		}

		if (previous_icon) {
			old_npoints = previous_icon->npoints;
			old_points = previous_icon->points;
		} else {
			old_npoints = 5;
			old_points = default_menu_drawing;
		}
		menu_animation.npoints = npoints;
		menu_animation.points = points;
		menu_animation.old_npoints = old_npoints;
		menu_animation.old_points = old_points;
		menu_animation.text_label_x = x;
	}

        cursor_x += (rect_w + CHAR_WIDTH);
        if (menu->attrib & LAST_ITEM) break;
        menu++;
	menu_item_number++;
	if (menu_item_number - menu_scroll_start_item >= MENU_MAX_ITEMS_DISPLAYABLE)
		break;
    } // END WHILE

    menu_animation.frame = 0;
    menu_animation.came_from = came_from;
    menu_animation.root_menu = root_menu;
    menu_animation.selected = selected;

    if (menu_animation.suppress_animation) {
        menu_animation.frame = 255;
	SUPPRESS_ANIMATION(0, "DM");
    }

    if (selected)
	previous_icon = selected->icon;
    else
	previous_icon = NULL;

    /* in case last menu item is a skip */
    if (selected == NULL) {
        selected = root_menu;
    }
    return selected;
}

struct menu_t *(*display_menu)(struct menu_t *menu, struct menu_t *selected,
			MENU_STYLE style, enum menu_previous came_from) = new_display_menu;

/* for this increment the units are menu items */
#define PAGESIZE 8

static void push_menu(const struct menu_t *menu)
{
	if (G_menuCnt == MAX_MENU_DEPTH)
		return;
	/* Push current menu and selection onto stack */
	G_menuStack[G_menuCnt].currMenu = G_currMenu;
	G_menuStack[G_menuCnt].selectedMenu = G_selectedMenu;
	G_menuStack[G_menuCnt].menu_scroll_start_item = menu_scroll_start_item;
	menu_scroll_start_item = 0;
	G_menuCnt++;
	G_currMenu = (struct menu_t *) menu; /* go into this menu */
	G_selectedMenu = (struct menu_t *) menu; /* first item of current menu */
}

static void pop_menu(int call_display_menu)
{
	if (G_menuCnt == 0)
		return; /* stack is empty, error or main menu */

	if (!menu_has_icons(G_currMenu))
		SUPPRESS_ANIMATION(1, "popmenu");
	G_menuCnt--;
	G_currMenu = G_menuStack[G_menuCnt].currMenu;
	G_selectedMenu = G_menuStack[G_menuCnt].selectedMenu;
	menu_scroll_start_item = G_menuStack[G_menuCnt].menu_scroll_start_item;
	if (call_display_menu)
		G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE, MENU_CHILD);
	menu_animation_in_progress = 1;
	menu_animation.frame = 0;
}

/*
   NOTE-
     apps will call this only via returnToMenus() below, but since this returns to the callback
     code will execute up the the function return()
*/
static void internalReturnToMenus(void)
{
    /* Clear any stray buttons left over from the app */
    (void) button_down_latches();

    if (G_currMenu == NULL) {
        G_currMenu = (struct menu_t *) main_m;
        G_selectedMenu = NULL;
        G_menuStack[G_menuCnt].currMenu = G_currMenu;
        G_menuStack[G_menuCnt].selectedMenu = G_selectedMenu;
    }
    menu_animation_in_progress = 1;
    menu_animation.frame = 0;
    G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE, MENU_UNKNOWN);
    runningApp = NULL;
}

/* This is called by apps */
void returnToMenus(void)
{
	SUPPRESS_ANIMATION(1, "rtm");
	internalReturnToMenus();
}

static char *menu_item_description = NULL;
static void display_menu_item_description(__attribute__((unused)) struct menu_t *item)
{
	FbColor(CYAN);
	FbBackgroundColor(BLACK);
	FbClear();
	FbMove(0, 0);
	FbWriteString(menu_item_description);
	FbSwapBuffers();

	int r0 = button_get_rotation(0);
	int r1 = button_get_rotation(1);
	int down_latches = button_down_latches();
	if (r0 || r1 || BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches) ||
#if BADGE_HAS_ROTARY_SWITCHES
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
#endif
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		internalReturnToMenus();
	}
}

extern unsigned char is_dormant;

static int user_made_selection(struct menu_t *menu, int down_latches)
{
	int selection_direction;
	if (display_menu == new_display_menu && menu_has_icons(menu))
		selection_direction = BADGE_BUTTON_DOWN;
	else
		selection_direction = BADGE_BUTTON_RIGHT;

	return	
#if BADGE_HAS_ROTARY_SWITCHES
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
#endif
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches) ||
		BUTTON_PRESSED(selection_direction, down_latches);
}

static int user_moved_to_previous_item(struct menu_t *menu, int down_latches,
		int rotary0, int rotary1)
{
    int previous_button;
    if (display_menu == new_display_menu && menu_has_icons(menu))
        previous_button = BADGE_BUTTON_LEFT;
    else
        previous_button = BADGE_BUTTON_UP;
    return BUTTON_PRESSED(previous_button, down_latches) || rotary0 < 0 || rotary1 < 0;
}

static int user_moved_to_next_item(struct menu_t *menu, int down_latches,
			int rotary0, int rotary1)
{
    int next_button;
    if (display_menu == new_display_menu && menu_has_icons(menu))
        next_button = BADGE_BUTTON_RIGHT;
    else
        next_button = BADGE_BUTTON_DOWN;
    return BUTTON_PRESSED(next_button, down_latches) || rotary0 < 0 || rotary1 < 0;
}

static int user_backed_out(struct menu_t *menu, int down_latches)
{
    int back_out_button;
    if (display_menu == new_display_menu && menu_has_icons(menu))
        back_out_button = BADGE_BUTTON_UP;
    else
        back_out_button = BADGE_BUTTON_LEFT;

    return
#if BADGE_HAS_ROTARY_SWITCHES
	/* Left rotary encoder switch can be used to back out of menus */
	BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
#endif
	BUTTON_PRESSED(back_out_button, down_latches);
}

void menus()
{
    if (menu_animation_in_progress) {
	animate_menu(&menu_animation);
	return;
    }
    if (runningApp != NULL && !is_dormant) { /* running app is set by menus() */
	/* Call the runningApp if non-NULL and the screen saver is not active */
        (*runningApp)(NULL);
        return;
    }

    if (G_currMenu == NULL || (menu_redraw_main_menu)){
        menu_redraw_main_menu = 0;
        G_menuStack[G_menuCnt].currMenu = (struct menu_t *) main_m;
        G_menuStack[G_menuCnt].selectedMenu = NULL;
        G_currMenu = (struct menu_t *)main_m;
        //selectedMenu = G_currMenu;
        G_selectedMenu = NULL;
        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE, MENU_UNKNOWN);
    }

    int down_latches = button_down_latches();
    int rotary0 = button_get_rotation(0);
    int rotary1 = button_get_rotation(1);
    /* see if physical button has been clicked */
    if (user_made_selection(G_currMenu, down_latches)) {
	int via_back_item = 0;
        // action happened that will result in menu redraw
        // do_animation = 1;
        switch (G_selectedMenu->type) {

            case MORE: /* jump to next page of menu */
                menu_beep(MORE_FREQ); /* a */
                G_currMenu += PAGESIZE;
                G_selectedMenu = G_currMenu;
                break;

            case BACK: { /* return from menu */
		int suppress_animation = 0;
                menu_beep(BACK_FREQ);
		if (!menu_has_icons(G_currMenu)) {
			suppress_animation = 1;
		}
		pop_menu(0);
		if (suppress_animation)
			SUPPRESS_ANIMATION(1, "BCK2");
		via_back_item = 1;
                if (G_menuCnt == 0) {
			menu_animation.came_from = MENU_UNKNOWN;
			return; /* stack is empty, error or main menu */
		}
                break;
		}
            case TEXT: /* maybe highlight if clicked?? */
                menu_beep(TEXT_FREQ); /* c */
                break;

            case MENU: /* drills down into menu if clicked */
                menu_beep(MENU_FREQ); /* d */
		push_menu(G_selectedMenu->data.menu);
                break;

            case FUNCTION: /* call the function pointer if clicked */
                menu_beep(FUNC_FREQ); /* e */
                runningApp = G_selectedMenu->data.func;
                break;

	    case ITEM_DESC:
		menu_beep(TEXT_FREQ);
		menu_item_description = G_selectedMenu->data.description;
		runningApp = display_menu_item_description;
		break;

            default:
                break;
        }
        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE,
			via_back_item ? MENU_UNKNOWN : MENU_PARENT);
    } else if (user_moved_to_previous_item(G_currMenu, down_latches, rotary0, rotary1)) {
        /* handle slider/soft button clicks */
        menu_beep(TEXT_FREQ); /* f */
	int skip_back_menu_items = display_menu == new_display_menu && menu_has_icons(G_currMenu);
	G_selectedMenu = find_prev_menu_item(G_currMenu, G_selectedMenu,
				SKIP_ITEM, skip_back_menu_items);
	maybe_scroll_to(G_selectedMenu, G_currMenu); /* Scroll up if necessary */
	G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE, MENU_NEXT);
    } else if (user_moved_to_next_item(G_currMenu, down_latches, rotary0, rotary1)) {
        menu_beep(MORE_FREQ); /* g */
	int skip_back_menu_items = display_menu == new_display_menu && menu_has_icons(G_currMenu);
	G_selectedMenu = find_next_menu_item(G_currMenu, G_selectedMenu,
				SKIP_ITEM, skip_back_menu_items);
	maybe_scroll_to(G_selectedMenu, G_currMenu);
	G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE, MENU_PREVIOUS);
    } else if (user_backed_out(G_currMenu, down_latches)) {
	int suppress_animation = !menu_has_icons(G_currMenu);
        menu_beep(BACK_FREQ);
        pop_menu(0);
	if (suppress_animation)
		SUPPRESS_ANIMATION(1, "BCK3A");
        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE,
			MENU_CHILD);
        if (G_menuCnt == 0)
            return; /* stack is empty, error or main menu */
    }
}

void select_new_menu_style(__attribute__((unused)) struct menu_t *m)
{
	display_menu = new_display_menu;
	internalReturnToMenus();
}

void select_legacy_menu_style(__attribute__((unused)) struct menu_t *m)
{
	display_menu = legacy_display_menu;
	internalReturnToMenus();
}

void select_menu_speed_fast(__attribute__((unused)) struct menu_t *m)
{
	menu_animation_speed = 40;
	flash_kv_store_int("MENU_SPEED", menu_animation_speed);
	internalReturnToMenus();
}

void select_menu_speed_medium(__attribute__((unused)) struct menu_t *m)
{
	menu_animation_speed = 20;
	flash_kv_store_int("MENU_SPEED", menu_animation_speed);
	internalReturnToMenus();
}

void select_menu_speed_slow(__attribute__((unused)) struct menu_t *m)
{
	menu_animation_speed = 10;
	flash_kv_store_int("MENU_SPEED", menu_animation_speed);
	internalReturnToMenus();
}

