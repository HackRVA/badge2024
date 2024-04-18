#ifndef NEW_BADGE_MONSTERS_H
#define NEW_BADGE_MONSTERS_H

#include <stdbool.h>
#include <stddef.h>
#include "menu.h"
#include "dynmenu.h"

void badge_monsters_cb(struct menu_t *m);
void enable_monster(const int monster_id);
void render_screen_save_monsters(void);

#endif // NEW_BADGE_MONSTERS_H
