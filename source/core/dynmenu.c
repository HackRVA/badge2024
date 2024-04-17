
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "dynmenu.h"
#include <string.h>
#include <stdio.h>

void dynmenu_init(struct dynmenu *dm, struct dynmenu_item *item, unsigned char max_items)
{
	dm->item = item;
	dm->max_items = max_items;
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
}

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

void dynmenu_add_item(struct dynmenu *dm, char *text, int next_state, unsigned char cookie)
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
		if (i == dm->current_item)
			FbColor(GREEN);
		else
			FbColor(WHITE);
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

