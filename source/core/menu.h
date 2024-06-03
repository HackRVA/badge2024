#ifndef menu_h
#define menu_h

#include <stdint.h>

#include <badge.h>

/* Must ... resist ... temptation ... to rewrite ... all of whatever this is. */

/*
   low order bits of attrib can be used to store
   a number from 0..255 
*/
enum attrib_bits {
    VERT_BIT=9, /* max 16 bits in unsigned short */
    SKIP_BIT, /* skip when scrolling */
    DEFAULT_BIT,
    LAST_BIT,
};

#define VERT_ITEM    (1 << VERT_BIT)
#define SKIP_ITEM    (1 << SKIP_BIT)
#define DEFAULT_ITEM (1 << DEFAULT_BIT)
#define LAST_ITEM (1 << LAST_BIT)

enum menu_item_type {
   TEXT = 0,   /* text to display */
   BACK,    /* return to previous menu */
   MENU,    /* sub menu type */
   FUNCTION, /* c function */
   ITEM_DESC, /* Longer text description of item */
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
   enum menu_item_type type;
   union { /* when initializing the union, use designated
	    * initializers, e.g, "{ .menu = blah }", "{ .func = blah }" */
      const struct menu_t *menu;
      void (*func)(struct badge_app *);
      char *description;
      void *generic;
   } data;
   struct menu_icon *icon;
};

#if 0
/* nothing uses this */
struct menu_t *getMenuStack(unsigned char item);
#endif
/* This is only used by settings.c to change the name of menu items in LCD backlight menu */
struct menu_t *getSelectedMenuStack(unsigned char item);

/* Passed to menu display function so it knows which way we're coming from */
enum menu_previous {
    MENU_PREVIOUS = 0,	/* user came from previous menu item, e.g. scrolling down on legacy style menus */
    MENU_NEXT,		/* user came from next menu item, e.g. scrolling up on legacy style menus */
    MENU_PARENT,	/* user came from parent menu, e.g. pressing select, or right on legacy menus */
    MENU_CHILD,		/* user came from child menu, e.g. selecting "Back" */
    MENU_UNKNOWN,	/* unknown.  Used by blinkenlights, which does weird things. */
};

/* The display_menu is a function pointer which is set to either new_display_menu or legacy_display_menu
 * depending on which menu style is chosen by the user.
 */
extern struct menu_t *(*display_menu)(struct menu_t *menu, struct menu_t *selected, MENU_STYLE style,
			enum menu_previous came_from);

void returnToMenus(void);
void menus(void);

struct menu_t *getCurrMenu(void);
struct menu_t *getSelectedMenu(void);

extern void (*runningApp)(struct menu_t *menu) ;
void genericMenu(struct menu_t *L_menu, MENU_STYLE, uint32_t button_latches);

/* used by badge.c when going from dormant -> not dormant to redraw the main menu */
extern unsigned char menu_redraw_main_menu;
void select_legacy_menu_style(struct menu_t *m);
void select_new_menu_style(struct menu_t *m);
void select_menu_speed_fast(struct menu_t *m);
void select_menu_speed_medium(struct menu_t *m);
void select_menu_speed_slow(struct menu_t *m);
void enable_down_as_select(struct menu_t *m);
void disable_down_as_select(struct menu_t *m);

#endif
