/*
   simple menu system
   Author: Paul Bruggeman
   paul@Killercats.com
*/

#include "menu.h"
#include "settings.h"
#include "colors.h"
#include "ir.h"
#include "assetList.h"
#include "button.h"
#include "framebuffer.h"
#include "display_s6b33.h"
#include "audio.h"
#include "led_pwm.h"
#include "assets.h"

// Apps
#include "about_badge.h"
#include "badge_monsters.h"
#include "blinkenlights.h"
#include "conductor.h"
#include "cube.h"
#include "game_of_life.h"
#include "hacking_simulator.h"
#include "lunarlander.h"
#include "qc.h"
#include "spacetripper.h"
#include "smashout.h"
#include "username.h"
#include "slot_machine.h"

/* TODO: remove this warning once we get new 134x162 display hardware working. */
#if (LCD_XSIZE == 132 && LCD_YSIZE == 132)
#warning "Using old display size of 132x132, badge apps designed for new 134x162 display will likely crash."
 /* Adding a warning here so that if people write badge apps for the new display
  * size and try to run them on an old badge, they'll have a big fat clue why it's
  * likely crashing.  The warning is here instead of in display/framebuffer.h
  * so that it appears only once during compilation.
 */
#endif

#define MAIN_MENU_BKG_COLOR GREY2

unsigned char redraw_main_menu = 0;

void rvasec_splash_cb();

extern const struct menu_t games_m[];
extern const struct menu_t main_m[] ;
extern const struct menu_t settings_m[];
extern const struct menu_t schedule_m[];

#define NOTEDUR 100

#ifdef QC
 void (*runningApp)() = QC_cb;
#else
 void (*runningApp)() = rvasec_splash_cb;
#endif


#define MORE_INC 4

struct menuStack_t {
   struct menu_t *selectedMenu;
   struct menu_t *currMenu;
};

#define MAX_MENU_DEPTH 8
static unsigned char G_menuCnt=0; // index for G_menuStack

struct menuStack_t G_menuStack[MAX_MENU_DEPTH] = { {0,0} }; // track user traversing menus

struct menu_t *G_selectedMenu = NULL; /* item the cursor is on */
struct menu_t *G_currMenu = NULL; /* init */


struct menu_t *getSelectedMenu() {
    return G_selectedMenu;
}

struct menu_t *getCurrMenu() {
    return G_currMenu;
}

struct menu_t *getMenuStack(unsigned char item) {
   if (item > G_menuCnt) return 0;

   return G_menuStack[G_menuCnt-item].currMenu;
}

struct menu_t *getSelectedMenuStack(unsigned char item) {
   if (item > G_menuCnt) return 0;

   return G_menuStack[G_menuCnt-item].selectedMenu;
}

/*
  currently the char routine draws Y in decreasing (up), 
  so 1st Y position has to offset down CHAR_HEIGHT to account for that
*/

extern struct framebuffer_t G_Fb;
unsigned char menu_left=5;

/* these should all be variables or part of a theme */
#define MENU_LEFT menu_left
#define CHAR_WIDTH 9
#define CHAR_HEIGHT 8
#define SCAN_BLANK 1 /* blank lines between text entries */
#define TEXTRECT_OFFSET 1 /* text offset within rectangle */

#define RGBPACKED(R,G,B) ( ((unsigned short)(R)<<11) | ((unsigned short)(G)<<6) | (unsigned short)(B) )
struct menu_t *display_menu(struct menu_t *menu,
                            struct menu_t *selected,
                            MENU_STYLE style) {
    static unsigned char cursor_x, cursor_y;
    unsigned char c;
    struct menu_t *root_menu; /* keep a copy in case menu has a bad structure */

    root_menu = menu;

    switch (style) {
        case MAIN_MENU_STYLE:
            FbBackgroundColor(MAIN_MENU_BKG_COLOR);
            FbClear();

            FbColor(GREEN);
            FbMove(2,5);
            FbRectangle(123, 120);

            FbColor(CYAN);
            FbMove(1,4);
            FbRectangle(125, 122);
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

        for (c=0, rect_w=0; (menu->name[c] != 0); c++) rect_w += CHAR_WIDTH;

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

void closeMenuAndReturn() {
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
    if (G_currMenu == NULL) {
        G_currMenu = (struct menu_t *)main_m;
        G_selectedMenu = NULL;
        G_menuStack[G_menuCnt].currMenu = G_currMenu;
        G_menuStack[G_menuCnt].selectedMenu = G_selectedMenu;
    }

    G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
    runningApp = NULL;
}

void menus() {
    if (runningApp != NULL) { /* running app is set by menus() not genericMenus() */
        (*runningApp)();
        return;
    }

    if (G_currMenu == NULL || (redraw_main_menu)){
        redraw_main_menu = 0;
        G_menuStack[G_menuCnt].currMenu = (struct menu_t *)main_m;
        G_menuStack[G_menuCnt].selectedMenu = NULL;
        G_currMenu = (struct menu_t *)main_m;
        //selectedMenu = G_currMenu;
        G_selectedMenu = NULL;
        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
    }

    int down_latches = button_down_latches();
    int rotary = button_get_rotation(0);
    /* see if physical button has been clicked */
    if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
        // action happened that will result in menu redraw
        // do_animation = 1;
        switch (G_selectedMenu->type) {

            case MORE: /* jump to next page of menu */
                audio_out_beep(1000, NOTEDUR); /* a */
                G_currMenu += PAGESIZE;
                G_selectedMenu = G_currMenu;
                break;

            case BACK: /* return from menu */
                audio_out_beep(1200, NOTEDUR);
                if (G_menuCnt == 0) return; /* stack is empty, error or main menu */
                G_menuCnt--;
                G_currMenu = G_menuStack[G_menuCnt].currMenu ;
                G_selectedMenu = G_menuStack[G_menuCnt].selectedMenu ;
                //G_selectedMenu = G_currMenu;
                break;

            case TEXT: /* maybe highlight if clicked?? */
                audio_out_beep(1400, NOTEDUR); /* c */
                break;

            case MENU: /* drills down into menu if clicked */
                audio_out_beep(1600, NOTEDUR); /* d */
                G_menuStack[G_menuCnt].currMenu = G_currMenu; /* push onto stack  */
                G_menuStack[G_menuCnt].selectedMenu = G_selectedMenu;
                G_menuCnt++;
                if (G_menuCnt == MAX_MENU_DEPTH) G_menuCnt--; /* too deep, undo */
                G_currMenu = (struct menu_t *)G_selectedMenu->data.menu; /* go into this menu */
                //selectedMenu = G_currMenu;
                G_selectedMenu = NULL;
                break;

            case FUNCTION: /* call the function pointer if clicked */
                audio_out_beep(1800, NOTEDUR); /* e */
                runningApp = G_selectedMenu->data.func;
                //(*selectedMenu->data.func)();
                break;

            default:
                break;
        }

        G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
    } else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0) {
        /* handle slider/soft button clicks */
        audio_out_beep(1400, NOTEDUR); /* f */

        /* make sure not on first menu item */
        if (G_selectedMenu > G_currMenu) {
            G_selectedMenu--;

            while ( ((G_selectedMenu->attrib & SKIP_ITEM) || (G_selectedMenu->attrib & HIDDEN_ITEM))
                    && G_selectedMenu > G_currMenu) {
                G_selectedMenu--;
            }

            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        } else {
            /* Move to the last item if press UP from the first item */
            while (!(G_selectedMenu->attrib & LAST_ITEM)) {
                G_selectedMenu++;
            }
            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        }
    } else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0) {
        audio_out_beep(1000, NOTEDUR); /* g */

        /* make sure not on last menu item */
        if (!(G_selectedMenu->attrib & LAST_ITEM)) {
            G_selectedMenu++;

            //Last item should never be a skipped item!!
            while ( ((G_selectedMenu->attrib & SKIP_ITEM) || (G_selectedMenu->attrib & HIDDEN_ITEM))
                    && (!(G_selectedMenu->attrib & LAST_ITEM)) ) {
                G_selectedMenu++;
            }

            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        } else {
            /* Move to the first item if press DOWN from the last item */
            while (G_selectedMenu > G_currMenu) {
                G_selectedMenu--;
            }
            G_selectedMenu = display_menu(G_currMenu, G_selectedMenu, MAIN_MENU_STYLE);
        }
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

    if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
        switch (L_selectedMenu->type) {
            case MORE: /* jump to next page of menu */
                audio_out_beep(1000, NOTEDUR); /* a */
                L_currMenu += PAGESIZE;
                L_selectedMenu = L_currMenu;
                break;

            case BACK: /* return from menu */
                audio_out_beep(1200, NOTEDUR); /* b */
                if (L_menuCnt == 0) return; /* stack is empty, error or main menu */
                L_menuCnt--;
                L_currMenu = L_menuStack[L_menuCnt] ;
                L_selectedMenu = L_currMenu;
                L_selectedMenu = display_menu(L_currMenu, L_selectedMenu, style);
                break;

            case TEXT: /* maybe highlight if clicked?? */
                audio_out_beep(1400, NOTEDUR); /* c */
                break;

            case MENU: /* drills down into menu if clicked */
                audio_out_beep(1600, NOTEDUR); /* d */
                L_menuStack[L_menuCnt++] = L_currMenu; /* push onto stack  */
                if (L_menuCnt == MAX_MENU_DEPTH) L_menuCnt--; /* too deep, undo */
                L_currMenu = (struct menu_t *)L_selectedMenu->data.menu; /* go into this menu */
                //L_selectedMenu = L_currMenu;
                L_selectedMenu = NULL;
                L_selectedMenu = display_menu(L_currMenu, L_selectedMenu, style);
                break;

            case FUNCTION: /* call the function pointer if clicked */
                audio_out_beep(1800, NOTEDUR); /* e */
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
        audio_out_beep(1400, NOTEDUR); /* f */

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
        audio_out_beep(1000, NOTEDUR); /* g */

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

const struct menu_t games_m[] = {
   {"Blinkenlights", VERT_ITEM|DEFAULT_ITEM, FUNCTION, {(struct menu_t *)blinkenlights_cb}}, // Set other badges LED
   {"Conductor",     VERT_ITEM, FUNCTION, {(struct menu_t *)conductor_cb}}, // Tell other badges to play notes
   {"Lunar Rescue",  VERT_ITEM, FUNCTION, {(struct menu_t *)lunarlander_cb} },
   {"Badge Monsters",VERT_ITEM, FUNCTION, {(struct menu_t *)badge_monsters_cb} },
   {"Smashout",      VERT_ITEM, FUNCTION, {(struct menu_t *)smashout_cb} },
   {"Hacking Sim",   VERT_ITEM, FUNCTION, {(struct menu_t *)hacking_simulator_cb} },
   {"Spinning Cube", VERT_ITEM, FUNCTION, {(struct menu_t *)cube_cb} },
   {"Game of Life", VERT_ITEM, FUNCTION, {(struct menu_t *)game_of_life_cb} },
   {"Slot Machine", VERT_ITEM, FUNCTION, {(struct menu_t *)slot_machine_cb}},
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t settings_m[] = {
   {"Backlight", VERT_ITEM, MENU, {(struct menu_t *)backlight_m}},
   {"Led", VERT_ITEM, MENU, {(struct menu_t *)LEDlight_m}},  /* coerce/cast to a menu_t data pointer */
   {"Audio", VERT_ITEM|DEFAULT_ITEM, MENU, {(struct menu_t *)buzzer_m}},
   {"Rotate", VERT_ITEM, MENU, {(struct menu_t *)rotate_m}},
   {"User Name", VERT_ITEM, FUNCTION, {(struct menu_t *)username_cb} },
   {"Screensaver", VERT_ITEM, MENU, {(struct menu_t *)screen_lock_m} },
   {"ID", VERT_ITEM, MENU, {(struct menu_t *)myBadgeid_m}},
   {"QC",  VERT_ITEM, FUNCTION, {(struct menu_t *)QC_cb}},
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t main_m[] = {
   {"Schedule",    VERT_ITEM, MENU, {schedule_m}},
   {"Games",       VERT_ITEM|DEFAULT_ITEM, MENU, {games_m}},
   {"Settings",    VERT_ITEM, MENU, {settings_m}},
   {"About Badge",    VERT_ITEM|LAST_ITEM, FUNCTION, {(struct menu_t *) about_badge_cb}},
} ;

const char splash_words1[] = "Loading";
#define NUM_WORD_THINGS 18
const char *splash_word_things[] = {"Cognition Module",
    "useless bits",
    "backdoor.sh",
    "exploit inside",
    "sub-zero",
    "lifting tables",
    "personal data",
    "important bits",
    "bitcoin miner",
    "GozNym",
    "broken feature",
    "NTFS", "Wall hacks",
    "huawei 5G",
    "Key logger",
    "badgedows defender", "sshd", "cryptolocker", };
    
const char splash_words_btn1[] = "Press the button";
const char splash_words_btn2[] = "to continue!";

#define SPLASH_SHIFT_DOWN 85
void rvasec_splash_cb(){
    static unsigned short wait = 0;
    static unsigned char loading_txt_idx = 0,
    load_bar = 0;

    if (wait == 0) {
        load_bar = 10;
        S6B33_rect(0, 0, LCD_XSIZE, LCD_YSIZE);
        S6B33_color(0);
        FbSwapBuffers();
        led_pwm_enable(BADGE_LED_RGB_GREEN, 50 * 255/100);
        //if(buzzer)
        audio_out_beep(1000, NOTEDUR);
    } else if(wait < 40){
        drawLCD4(HACKRVA4, 0);
        FbSwapBuffers();
        //PowerSaveIdle();
    } else if(wait < 80){
        FbMove(0, 2);
        FbImage2bit(&assetList[RVASEC2016], 0);
        FbMove(10,SPLASH_SHIFT_DOWN);

        FbColor(WHITE);
        FbRectangle(100, 20);

        FbMove(35, SPLASH_SHIFT_DOWN - 13);
        FbColor(YELLOW);
        FbWriteLine(splash_words1);

        FbMove(11, SPLASH_SHIFT_DOWN+1);
        FbColor(GREEN);
        FbFilledRectangle((load_bar++ << 1) + 1,19);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 10 * 255 / 100);

        FbColor(WHITE);
        FbMove(4, 113);
        FbWriteLine(splash_word_things[loading_txt_idx%NUM_WORD_THINGS]);
        if(!(wait%2))
            loading_txt_idx++;

        FbSwapBuffers();

    } else if(wait <160){
        FbMove(0, 2);
        FbImage2bit(&assetList[RVASEC2016], 0);
        FbMove(10,SPLASH_SHIFT_DOWN);
        FbColor(GREEN);
        FbLine(0,60,132,60);
        FbLine(0,62,132,62);
        FbLine(0,65,132,65);
        FbLine(0,69,132,69);
        FbLine(0,77,132,77);

        FbLine(105,60,145,77);
        FbLine(95, 60,125,77);
        FbLine(85, 60,105,77);
        FbLine(75, 60,85,77);
        FbLine(65, 60,65,77);
        FbLine(55, 60,45,77);
        FbLine(45, 60,25,77);
        FbLine(35, 60,5,77);
        FbLine(25, 60,0,65);

        FbMove(1, 90);
        FbWriteLine(splash_words_btn1);

        FbMove(15, 110);
        FbWriteLine(splash_words_btn2);

        FbSwapBuffers();
    }

    wait++;
    if(wait == (sizeof(unsigned short))-2) {
        wait -= 1000;
    }

    // Sam: had some buzzer code here prior
    int down_latches = button_down_latches();

    if(BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)){
        returnToMenus();
    }
}


