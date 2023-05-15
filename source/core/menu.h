#ifndef menu_h
#define menu_h

#include <stdint.h>

/* Must ... resist ... temptation ... to rewrite ... all of whatever this is. */

/*
   low order bits of attrib can be used to store
   a number from 0..255 
*/
enum attrib_bits {
    VERT_BIT=9, /* max 16 bits in unsigned short */
    HORIZ_BIT,
    SKIP_BIT, /* skip when scrolling */
    DEFAULT_BIT,
    HIDDEN_BIT,
    LAST_BIT,
};

#define VERT_ITEM    (1 << VERT_BIT)
#define HORIZ_ITEM   (1 << HORIZ_BIT)
#define SKIP_ITEM    (1 << SKIP_BIT)
#define DEFAULT_ITEM (1 << DEFAULT_BIT)
#define HIDDEN_ITEM  (1 << HIDDEN_BIT)
#define LAST_ITEM (1 << LAST_BIT)

enum type {
   MORE=0, /* if the menu is too long to fit (but menus scroll now, so you shouldn't need this) */
   TEXT,   /* text to display */
   BACK,    /* return to previous menu */
   MENU,    /* sub menu type */
   FUNCTION /* c function */
};

typedef enum  {
    MAIN_MENU_STYLE,
    MAIN_MENU_WITH_TIME_DATE_STYLE,
    DRBOB_MENU_STYLE,
    WHITE_ON_BLACK,
    BLANK
} MENU_STYLE;

struct menu_t {
   char name[16];
   unsigned short attrib;
   unsigned char type;
   union { /* when initializing the union, use designated
	    * initializers, e.g, "{ .menu = blah }", "{ .func = blah }" */
      const struct menu_t *menu;
      void (*func)(struct menu_t *m);
      void *generic;
   } data;
};

#if 0
/* nothing uses this */
struct menu_t *getMenuStack(unsigned char item);
#endif
/* This is only used by settings.c to change the name of menu items in LCD backlight menu */
struct menu_t *getSelectedMenuStack(unsigned char item);

struct menu_t *display_menu(struct menu_t *menu, struct menu_t *selected, MENU_STYLE style);
void returnToMenus(void);
void menus(void);

struct menu_t *getCurrMenu(void);
struct menu_t *getSelectedMenu(void);

extern void (*runningApp)(struct menu_t *menu) ;
void genericMenu(struct menu_t *L_menu, MENU_STYLE, uint32_t button_latches);
void closeMenuAndReturn(void);

/* used by badge.c when going from dormant -> not dormant to redraw the main menu */
extern unsigned char menu_redraw_main_menu;

#endif
