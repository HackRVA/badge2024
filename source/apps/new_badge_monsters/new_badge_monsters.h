#ifndef NEW_BADGE_MONSTERS_H
#define NEW_BADGE_MONSTERS_H

#include <stdbool.h>
#include <stddef.h>
typedef char colormap[16][3];

struct new_monster
{
    char name[20];
    bool owned;
    int color;
    char blurb[128];
    const struct asset *asset;
};

extern const struct asset new_badge_monsters_godzilla;
extern const struct asset new_badge_monsters_gopher;

void app_init(void);
void exit_app(void);
char *key_from_monster(const struct new_monster *m, char *key, size_t len);
void set_monster_owned(const int monster_id, const bool owned);
void enable_monster(const int monster_id);
// loads all monsters from flash key-value store
void load_from_flash();
void save_to_flash();


//***************** MENUS ******************
enum menu_level_t {
    MAIN_MENU,
    MONSTER_MENU,
    DESCRIPTION
};

void change_menu_level(enum menu_level_t level);
void draw_menu(void);
void check_the_buttons(void);
void setup_monster_menu(void);
void setup_main_menu(void);
void game_menu(void);
void render_monster(void);
void render_screen_save_monsters(void);
void show_message(const char *message);

#ifdef __linux__
void enable_all_monsters(void);
void print_menu_info(void);
#endif

#endif // NEW_BADGE_MONSTERS_H
