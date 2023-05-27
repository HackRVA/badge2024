
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
	dm->chosen_cookie = 0;
}

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

void dynmenu_add_item(struct dynmenu *dm, char *text, int next_state, unsigned char cookie)
{
    int i;

    if (dm->nitems >= dm->max_items) {
#ifdef __linux__
        printf("dynmenu_add_item: WARNING: failed to add menu item %d, max_items = %d\n", dm->nitems, dm->max_items);
        printf("menu title: %s\n", dm->title);
#endif
        return;
    }

    i = dm->nitems;
#ifdef __linux__
    printf("dynmenu_add_item: '%s', length: %lu\n", text, sizeof(dm->item[i].text)-1);
#endif
    strncpy(dm->item[i].text, text, sizeof(dm->item[i].text) - 1);
    dm->item[i].next_state = next_state;
    dm->item[i].cookie = cookie;
    dm->nitems++;
}

void dynmenu_draw(struct dynmenu *dm)
{
	int i, y, first_item, last_item;

	first_item = dm->current_item - 3;
	if (first_item < 0)
		first_item = 0;
	last_item = dm->current_item + 3;
	if (last_item > dm->nitems - 1)
		last_item = dm->nitems - 1;

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

	y = LCD_YSIZE / 2 - 10 * (dm->current_item - first_item);
	for (i = first_item; i <= last_item; i++) {
		if (i == dm->current_item)
			FbColor(GREEN);
		else
			FbColor(WHITE);
		FbMove(10, y);
		FbWriteLine(dm->item[i].text);
		y += 10;
	}

	FbColor(GREEN);
	FbMove(5, LCD_YSIZE / 2 - 2);
	FbRectangle(LCD_XSIZE - 7, 12);
}

void dynmenu_change_current_selection(struct dynmenu *dm, int direction)
{
	int new = dm->current_item + direction;
	if (new < 0)
		new = dm->nitems - 1;
	else if (new >= dm->nitems)
		new = 0;
	dm->current_item = new;
}

