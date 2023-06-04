/*
   simple menu system
   Author: Paul Bruggeman
   paul@Killercats.com
*/
#include <stdio.h>
#include <stdlib.h>
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

// Apps
#include "about_badge.h"
#include "badge_monsters.h"
#include "battlezone.h"
#include "clue.h"
#include "game_of_life.h"
#include "hacking_simulator.h"
#include "lunarlander.h"
#include "pong.h"
#include "qc.h"
#include "smashout.h"
#include "username.h"
#include "slot_machine.h"
#include "gulag.h"
#include "asteroids.h"
#include "etch-a-sketch.h"
#include "magic-8-ball.h"
#include "rvasec_splash.h"
#include "test-screensavers.h"
#include "tank-vs-tank.h"

#define MAIN_MENU_BKG_COLOR GREY2

extern const struct menu_t schedule_m[]; /* defined in core/schedule.c */

static const struct menu_t games_m[] = {
   {"Battlezone", VERT_ITEM|DEFAULT_ITEM, FUNCTION, { .func = battlezone_cb }, },
   {"Asteroids", VERT_ITEM, FUNCTION, { .func = asteroids_cb }, },
   {"Etch-a-Sketch", VERT_ITEM, FUNCTION, { .func = etch_a_sketch_cb }, },
   {"Magic-8-Ball",     VERT_ITEM, FUNCTION, { .func = magic_8_ball_cb }, },
   {"Goodbye Gulag", VERT_ITEM, FUNCTION, { .func = gulag_cb }, },
   {"Clue", VERT_ITEM, FUNCTION, { .func = clue_cb }, },
   {"Pong", VERT_ITEM, FUNCTION, { .func = pong_cb }, },
   {"Tank vs Tank", VERT_ITEM, FUNCTION, { .func = tank_vs_tank_cb }, },
   {"Lunar Rescue",  VERT_ITEM, FUNCTION, { .func = lunarlander_cb}, },
   {"Badge Monsters",VERT_ITEM, FUNCTION, { .func = badge_monsters_cb }, },
   {"Smashout",      VERT_ITEM, FUNCTION, { .func = smashout_cb }, },
   {"Hacking Sim",   VERT_ITEM, FUNCTION, { .func = hacking_simulator_cb }, },
   {"Game of Life", VERT_ITEM, FUNCTION, { .func = game_of_life_cb }, },
   {"Slot Machine", VERT_ITEM, FUNCTION, { .func = slot_machine_cb }, },
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, { NULL }, },
};

static const struct menu_t settings_m[] = {
   {"Backlight", VERT_ITEM, MENU, { .menu = backlight_m }, },
   {"Led", VERT_ITEM, MENU, { .menu = LEDlight_m }, },
   {"Audio", VERT_ITEM|DEFAULT_ITEM, MENU, { .menu = buzzer_m }, },
   {"Invert Display", VERT_ITEM, MENU, { .menu = rotate_m, }},
   {"User Name", VERT_ITEM, FUNCTION, { .func = username_cb }, },
   {"Screensaver", VERT_ITEM, MENU, { .menu = screen_lock_m }, },
   {"ID", VERT_ITEM, MENU, { .menu = myBadgeid_m }, },
   {"QC",  VERT_ITEM, FUNCTION, { .func = QC_cb }, },
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

static const struct menu_t main_m[] = {
   {"Schedule",    VERT_ITEM, MENU, { .menu = schedule_m }, },
   {"Games",       VERT_ITEM|DEFAULT_ITEM, MENU, { .menu = games_m }, },
   {"Settings",    VERT_ITEM, MENU, { .menu = settings_m }, },
   {"About Badge",    VERT_ITEM|LAST_ITEM, FUNCTION, { .func = about_badge_cb }, },
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
		/* I think HIDDEN_ITEMs breaks this, but I don't think we use any HIDDEN_ITEMs
		 * and we should probably just delete the concept of HIDDEN_ITEMs */
		position = selected - current_menu;
	}
	if (position < menu_scroll_start_item)
		menu_scroll_start_item = position;
	if (position > menu_scroll_start_item + MENU_MAX_ITEMS_DISPLAYABLE - 1)
		menu_scroll_start_item = position - MENU_MAX_ITEMS_DISPLAYABLE + 1;
}

/* The reason that display_menu returns a menu_t * instead of void
   as you might expect is because sometimes it skips over unselectable
   items
 */
struct menu_t *display_menu(struct menu_t *menu,
                            struct menu_t *selected,
                            MENU_STYLE style) {
    static unsigned char cursor_x, cursor_y;
    unsigned char c;
    struct menu_t *root_menu; /* keep a copy in case menu has a bad structure */
    int menu_item_number = 0;

    root_menu = menu;

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

        if (menu->attrib & HIDDEN_ITEM) {
            // don't jump out of the menu array if this is the last item!
            if(menu->attrib & LAST_ITEM) {
                break;
            } else {
                menu++;
            }
            continue;
        }

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

static void pop_menu(void)
{
	if (G_menuCnt == 0)
		return; /* stack is empty, error or main menu */
	G_menuCnt--;
	G_currMenu = G_menuStack[G_menuCnt].currMenu;
	G_selectedMenu = G_menuStack[G_menuCnt].selectedMenu;
	menu_scroll_start_item = G_menuStack[G_menuCnt].menu_scroll_start_item;
        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
}

void closeMenuAndReturn(void) {
    if (G_menuCnt == 0) return; /* stack is empty, error or main menu */
    G_menuCnt--;
    G_currMenu = G_menuStack[G_menuCnt].currMenu ;
    G_selectedMenu = G_menuStack[G_menuCnt].selectedMenu ;
    G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
    runningApp = NULL;
}

/* 
   NOTE-
     apps will call this but since this returns to the callback
     code will execute up the the fuction return()
*/
void returnToMenus() {
    /* Clear any stray buttons left over from the app */
    (void) button_down_latches();

    if (G_currMenu == NULL) {
        G_currMenu = (struct menu_t *) main_m;
        G_selectedMenu = NULL;
        G_menuStack[G_menuCnt].currMenu = G_currMenu;
        G_menuStack[G_menuCnt].selectedMenu = G_selectedMenu;
    }

    G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
    runningApp = NULL;
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
	if (r0 || r1 ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		returnToMenus();
	}
}

extern unsigned char is_dormant;

void menus() {
    if (runningApp != NULL && !is_dormant) { /* running app is set by menus() not genericMenus() */
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
        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
    }

    int down_latches = button_down_latches();
    int rotary = button_get_rotation(0);
    /* see if physical button has been clicked */
    if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
	BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
        // action happened that will result in menu redraw
        // do_animation = 1;
        switch (G_selectedMenu->type) {

            case MORE: /* jump to next page of menu */
                menu_beep(MORE_FREQ); /* a */
                G_currMenu += PAGESIZE;
                G_selectedMenu = G_currMenu;
                break;

            case BACK: /* return from menu */
                menu_beep(BACK_FREQ);
		pop_menu();
                if (G_menuCnt == 0)
			return; /* stack is empty, error or main menu */
                break;

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

        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
    } else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0) {
        /* handle slider/soft button clicks */
        menu_beep(TEXT_FREQ); /* f */

        /* make sure not on first menu item */
        if (G_selectedMenu > G_currMenu) {
            G_selectedMenu--;

            while ( ((G_selectedMenu->attrib & SKIP_ITEM) || (G_selectedMenu->attrib & HIDDEN_ITEM))
                    && G_selectedMenu > G_currMenu) {
                G_selectedMenu--;
            }
            if (G_selectedMenu->attrib & SKIP_ITEM) { /* It seems the first item is a SKIP_ITEM */
		    while (!(G_selectedMenu->attrib & LAST_ITEM)) { /* Move to the last item */
			G_selectedMenu++;
		    }
            }
	    maybe_scroll_to(G_selectedMenu, G_currMenu); /* Scroll up if necessary */
            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        } else {
            /* Move to the last item if press UP from the first item */
            while (!(G_selectedMenu->attrib & LAST_ITEM)) {
                G_selectedMenu++;
            }
	    maybe_scroll_to(G_selectedMenu, G_currMenu);
            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        }
    } else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0) {
        menu_beep(MORE_FREQ); /* g */

        /* make sure not on last menu item */
        if (!(G_selectedMenu->attrib & LAST_ITEM)) {
            G_selectedMenu++;


            //Last item should never be a skipped item!!
            while ( ((G_selectedMenu->attrib & SKIP_ITEM) || (G_selectedMenu->attrib & HIDDEN_ITEM))
                    && (!(G_selectedMenu->attrib & LAST_ITEM)) ) {
                G_selectedMenu++;
            }

	    maybe_scroll_to(G_selectedMenu, G_currMenu);
            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        } else {
            /* Move to the first item if press DOWN from the last item */
            while (G_selectedMenu > G_currMenu) {
                G_selectedMenu--;
            }
	    maybe_scroll_to(G_selectedMenu, G_currMenu);
            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        }
    } else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
	/* Left rotary encoder switch can be used to back out of menus */
        menu_beep(BACK_FREQ);
        pop_menu();
        if (G_menuCnt == 0)
            return; /* stack is empty, error or main menu */
    }
}

/*
  ripped from above for app menus
  this is not meant for persistant menus
  like the main menu
*/
void genericMenu(struct menu_t *L_menu, MENU_STYLE style, uint32_t down_latches) {
    static struct menu_t *L_currMenu = NULL; /* LOCAL not to be confused to much with menu()*/
    static struct menu_t *L_selectedMenu = NULL; /* LOCAL ditto   "    "    */
    static unsigned char L_menuCnt=0; // index for G_menuStack
    static struct menu_t *L_menuStack[4] = { 0 }; // track user traversing menus

    if (L_menu == NULL) return; /* no thanks */

    if (L_currMenu == NULL) {
        L_menuCnt = 0;
        L_menuStack[L_menuCnt] = L_menu;
        L_currMenu = L_menu;
        //L_selectedMenu = L_menu;
        L_selectedMenu = NULL;
        L_selectedMenu = display_menu(L_currMenu, L_selectedMenu, style);
        return;
    }

    if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
        switch (L_selectedMenu->type) {
            case MORE: /* jump to next page of menu */
                menu_beep(MORE_FREQ); /* a */
                L_currMenu += PAGESIZE;
                L_selectedMenu = L_currMenu;
                break;

            case BACK: /* return from menu */
                menu_beep(BACK_FREQ); /* b */
                if (L_menuCnt == 0) return; /* stack is empty, error or main menu */
                L_menuCnt--;
                L_currMenu = L_menuStack[L_menuCnt] ;
                L_selectedMenu = L_currMenu;
                L_selectedMenu = display_menu(L_currMenu, L_selectedMenu, style);
                break;

            case TEXT: /* maybe highlight if clicked?? */
                menu_beep(TEXT_FREQ); /* c */
                break;

            case MENU: /* drills down into menu if clicked */
                menu_beep(MENU_FREQ); /* d */
                L_menuStack[L_menuCnt++] = L_currMenu; /* push onto stack  */
                if (L_menuCnt == MAX_MENU_DEPTH) L_menuCnt--; /* too deep, undo */
                L_currMenu = (struct menu_t *)L_selectedMenu->data.menu; /* go into this menu */
                //L_selectedMenu = L_currMenu;
                L_selectedMenu = NULL;
                L_selectedMenu = display_menu(L_currMenu, L_selectedMenu, style);
                break;

            case FUNCTION: /* call the function pointer if clicked */
                menu_beep(FUNC_FREQ); /* e */
                (*L_selectedMenu->data.func)(L_selectedMenu);

                /* clean up for nex call back */
                L_menu = NULL;
                L_currMenu = NULL;
                L_selectedMenu = NULL;

                L_menuCnt = 0;
                L_menuStack[L_menuCnt] = NULL;
                break;

            default:
                break;
        }
    } else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
        /* handle slider/soft button clicks */
        menu_beep(TEXT_FREQ); /* f */

        /* make sure not on first menu item */
        if (L_selectedMenu > L_currMenu) {
            L_selectedMenu--;

            while ((L_selectedMenu->attrib & SKIP_ITEM)
                    && L_selectedMenu > L_currMenu) {
                L_selectedMenu--;
            }

            L_selectedMenu = display_menu(L_currMenu, L_selectedMenu, style);
        }
    } else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
        menu_beep(MORE_FREQ); /* g */

        /* make sure not on last menu item */
        if (!(L_selectedMenu->attrib & LAST_ITEM)) {
            L_selectedMenu++;

            //Last item should never be a skipped item!!
            while (L_selectedMenu->attrib & SKIP_ITEM) {
                L_selectedMenu++;
            }

            L_selectedMenu = display_menu(L_currMenu, L_selectedMenu, style);
        }
    }
}

