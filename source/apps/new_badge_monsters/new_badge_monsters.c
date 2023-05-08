#include <stdint.h>
#include "new_badge_monsters.h"
#include "new_badge_monsters_assets.h"
#include "new_badge_monsters_ir.h"
#include "audio.h"
#include "badge.h"
#include "button.h"
#include "colors.h"
#include "framebuffer.h"
#include "key_value_storage.h"
#include "menu.h"
#include "stdio.h"
#include "string.h"
#include "utils.h"

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

enum app_states {
    INIT_APP_STATE,
    GAME_MENU,
    RENDER_SCREEN,
    RENDER_MONSTER,
    TRADE_MONSTERS,
    CHECK_THE_BUTTONS,
    EXIT_APP,
    ENABLE_ALL_MONSTERS
};

/***************************************** GLOBALS ************************************************/

struct new_monster new_monsters[] = {
    {
        "AccessGator",
        true,
        RED,
        "Hides in your access control config, waiting to bite you on the a**",
        &new_badge_monsters_assets_Access_Control_Alligator
    }, {
        "CryptoRaptor",
        false,
        YELLOW,
        "Attacks from above, out of the sun",
        &new_badge_monsters_assets_Crypto_Raptor
    }
};

/*
 * global state for game -- used by almost all functions.
*/
struct game_state {
    bool screen_changed;
    int smiley_x;
    int smiley_y;
    unsigned int current_monster;
    unsigned int nmonsters;
    enum app_states app_state;
    bool trading_monsters_enabled;
    unsigned int initial_mon;
    struct dynmenu menu;
    struct dynmenu_item menu_item[ARRAYSIZE(new_monsters)];
    enum menu_level_t menu_level;
};

static struct game_state state = {
    0, 0, 0, 0, 0, INIT_APP_STATE, false,
    0, {}, {}, MAIN_MENU
};

typedef void (*state_to_function_map_fn_type)();

static state_to_function_map_fn_type state_to_function_map[] = {
    app_init,
    game_menu,
    // render_screen,
    render_monster,
    // trade_monsters,
    check_the_buttons,
    exit_app,
#ifdef __linux__
    enable_all_monsters,
#endif
};

/***************************************** End of GLOBALS *****************************************/


void badge_monsters_cb(__attribute__((unused)) struct menu_t *m)
{
#ifdef __linux__
    printf("badge_monsters_cb: state %d\n", state.app_state);
#endif
    state_to_function_map[state.app_state]();
}

void app_init()
{
    FbInit();
    state.app_state = INIT_APP_STATE;
    register_ir_packet_callback(ir_packet_callback);
#ifdef __linux__
    printf("app_init: %lu, %lu\n", ARRAYSIZE(new_monsters), ARRAYSIZE(state.menu_item));
#endif
    dynmenu_init(&state.menu, state.menu_item, ARRAYSIZE(state.menu_item));

    change_menu_level(MAIN_MENU);
    state.app_state = GAME_MENU;
    state.screen_changed = true;
    state.smiley_x = LCD_XSIZE / 2; //?? name?
    state.smiley_y = LCD_XSIZE / 2;
    state.nmonsters = ARRAYSIZE(new_monsters);

    load_from_flash();
    state.initial_mon = badge_system_data()->badgeId % state.nmonsters;
    state.current_monster = state.initial_mon;
    enable_monster(state.initial_mon);
}

void exit_app(void)
{
    state.app_state = INIT_APP_STATE;
    save_to_flash();
    unregister_ir_packet_callback(ir_packet_callback);
    returnToMenus();
}

char *key_from_monster(const struct new_monster *m, char *key, size_t len)
{
    snprintf(key, len, "monster/%s", m->name);

    return key;
}

void load_from_flash()
{
    char key[20];
    int unused; //??
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
        if (!flash_kv_get_int(key_from_monster(m, key, sizeof(key)), &unused)) {
            m->owned = false;
        }
    }
}

void save_if_enabled(const struct new_monster *m)
{
    char key[20];

    if (m->owned) {
        flash_kv_store_int(key_from_monster(m, key, sizeof(key)), m->owned);
    }
}

void save_to_flash(void)
{
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
        save_if_enabled(m);
    }
}

void set_monster_owned(const int monster_id, const bool owned)
{
    new_monsters[monster_id].owned = owned;
}

void enable_monster(int monster_id)
{
    if(monster_id < 0 || (size_t) monster_id > ARRAYSIZE(new_monsters) - 1)
        return;

    new_monsters[monster_id].owned = true;
    audio_out_beep(900, 200);
    #ifdef __linux__
        printf("enabling monster: %d\n", monster_id);
    #endif
}

#ifdef __linux__
void enable_all_monsters(void)
{
	for (size_t i = 0; i < ARRAYSIZE(new_monsters); i++)
		enable_monster(i);
	FbClear();
	FbMove(2, 2);
	FbWriteString("ALL MONSTERS\nENABLED!");
	state.app_state = RENDER_SCREEN;
}
#endif


//********************************** MENUS *****************************************

void draw_menu(void)
{
    dynmenu_draw(&state.menu);
    if(state.menu_level != MONSTER_MENU){
        int nunlocked = 0;
        char available_monsters[3];
        char unlocked_monsters[3];
        for(size_t i = 0; i < ARRAYSIZE(new_monsters); i++)
        {
            if(new_monsters[i].owned)
            {
                nunlocked++;
            }
        }

        sprintf( available_monsters, "%lu", ARRAYSIZE(new_monsters));
        sprintf( unlocked_monsters, "%d", nunlocked);

        FbMove(1,25);
        FbWriteLine("Collected: ");
        FbWriteLine(unlocked_monsters);
        FbWriteLine("/");
        FbWriteLine(available_monsters);
    }
    state.app_state = RENDER_SCREEN;
}

void change_menu_level(enum menu_level_t level)
{
    static int which_item = -1;

    if (which_item == -1)
	    which_item = state.initial_mon;

    if (strcmp(state.menu.title, "Monsters") == 0)
	    which_item = state.menu.current_item;

    switch(level){
        case MAIN_MENU:
            setup_main_menu();
            state.screen_changed = true;
            break;
        case MONSTER_MENU:
            setup_monster_menu();
            if (state.menu.max_items > which_item)
		        state.menu.current_item = which_item; /* Stay on the same monster */
                state.current_monster = state.menu.item[state.menu.current_item].cookie;
            state.screen_changed = true;
            break;
        case DESCRIPTION:
            state.screen_changed = true;
            return;
    }
}

#ifdef __linux__
void print_menu_info()
{
    /* int next_state = menu.item[menu.current_item].next_state
       system("clear"); */
    printf("current item: %d\nmenu level: %d\ncurrent monster: %d\n menu item: %s\nn-menu-items: %d\ncookie_monster: %d\n",
           state.menu.current_item,
           state.menu_level,
           state.current_monster,
           state.menu.item[state.menu.current_item].text,
           state.menu.nitems,
           state.menu.item[state.menu.current_item].cookie);
}
#endif

void check_the_buttons(void)
{
    bool something_changed = false;
    int down_latches = button_down_latches();
    int rotary = button_get_rotation(0);

    /* If we are trading monsters, stop trading monsters on a button press */
    if (state.trading_monsters_enabled) {
		if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
			state.trading_monsters_enabled = false;
			state.app_state = GAME_MENU;
			state.screen_changed = true;
			return;
		}
    }

    switch(state.menu_level){
        case MAIN_MENU:
            if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
            {
                dynmenu_change_current_selection(&state.menu, -1);
		        state.screen_changed = true;
                something_changed = true;
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
            {
                dynmenu_change_current_selection(&state.menu, 1);
		        state.screen_changed = true;
                something_changed = true;
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches))
            {
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
            {
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
            {
                switch(state.menu.current_item){
                    case 0:
                        change_menu_level(MONSTER_MENU);
                        state.current_monster = state.menu.item[state.menu.current_item].cookie;
                        break;
                    case 1:
                        state.app_state = TRADE_MONSTERS;
                        break;
                    case 2:
                        state.app_state = EXIT_APP;
                        break;
#ifdef __linux__
		            case 3:
			            state.app_state = ENABLE_ALL_MONSTERS;
			            break;
#endif
                }
            }

            break;
        case MONSTER_MENU:
            if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
            {
                dynmenu_change_current_selection(&state.menu, -1);
		        state.screen_changed = true;
                state.current_monster = state.menu.item[state.menu.current_item].cookie;
                render_monster();
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
            {
                dynmenu_change_current_selection(&state.menu, 1);
		        state.screen_changed = true;
                state.current_monster = state.menu.item[state.menu.current_item].cookie;
                render_monster();
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
            {
                change_menu_level(MAIN_MENU);
                something_changed = true;
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
            {
                show_message(new_monsters[state.current_monster].blurb);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
            {
                #ifdef __linux__
                    print_menu_info();
                #endif
                show_message(new_monsters[state.current_monster].blurb);
             }
            break;
        case DESCRIPTION:
            if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
			BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
            {
                change_menu_level(MONSTER_MENU);
            }
            break;
        default:
            break;
        }

    if (state.trading_monsters_enabled && !something_changed) {
	    state.app_state = TRADE_MONSTERS;
	    return;
    }

    if (something_changed && state.app_state == CHECK_THE_BUTTONS)
        state.app_state = GAME_MENU;
    return;
}

void setup_monster_menu(void)
{
    dynmenu_clear(&state.menu);
    state.menu.menu_active = 0;
    strcpy(state.menu.title, "Monsters");

    int index = 0;
    for(const struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++){
        if(m->owned)
            dynmenu_add_item(&state.menu, (char *)m->name, RENDER_MONSTER, index);
        index++;
    }
    if (state.current_monster < state.menu.max_items)
        state.menu.current_item = state.current_monster;
    else
        state.current_monster = state.menu.current_item;

    state.screen_changed = true;
    render_monster();
}

void setup_main_menu(void)
{
    dynmenu_clear(&state.menu);
    strcpy(state.menu.title, "Badge Monsters");
    dynmenu_add_item(&state.menu, "Monsters", RENDER_SCREEN, 0);
    dynmenu_add_item(&state.menu, "Trade Monsters", TRADE_MONSTERS, 1);
    dynmenu_add_item(&state.menu, "EXIT", EXIT_APP, 2);
#ifdef __linux__
    /* For testing purposes allow enabling all monsters on linux */
    dynmenu_add_item(&state.menu, "Test Monsters", ENABLE_ALL_MONSTERS, 3);
#endif
    state.screen_changed = true;
}

void game_menu(void)
{
    state.menu_level = MAIN_MENU;
    draw_menu();
    check_for_incoming_packets();
    state.app_state = RENDER_SCREEN;
}

//**************************** DISPLAY ***********************************************

void render_monster(void)
{
    const struct new_monster monster = new_monsters[state.current_monster];

    FbClear();
    if(state.current_monster == state.initial_mon)
    {
        FbMove(0,10);
        FbWriteLine("--starting-mon--");
    }
    FbMove(0,0);
    FbColor(monster.color);
    FbWriteLine(monster.name);
    FbImage4bit(monster.asset, 0);

    FbColor(WHITE);
    FbMove(43,120);
    FbWriteLine("|down|");
    FbColor(GREEN);

    FbMove(5, 120);
    FbWriteLine("<Back");


    FbMove(90,120);
    FbWriteLine("desc>");

    FbSwapBuffers();
    state.screen_changed = true;
    state.app_state = RENDER_SCREEN;
}

void show_message(const char *message)
{
    #ifdef __linux__
        printf("%s\n", message);
    #endif

    FbClear();
    FbColor(WHITE);

    FbMove(8, 5);
    FbWriteString(message);
    FbMove(5, 120);
    FbWriteLine("<Back");

    change_menu_level(DESCRIPTION);
    state.app_state = RENDER_SCREEN;
    state.screen_changed = true;
}

void render_screen(void)
{
    state.app_state = CHECK_THE_BUTTONS;
    if (!state.screen_changed)
        return;
    FbPushBuffer();
    state.screen_changed = false;
}

void render_screen_save_monsters(void) {
    static unsigned char current_index = 0;
    current_index++;
    // wrap-around if out of bounds
    current_index %= ARRAYSIZE(new_monsters) - 1;

    FbClear();
    FbColor(BLACK);
    FbImage4bit(new_monsters[current_index].asset, 0);
}

