#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "new_badge_monsters.h"
#define DEFINE_IMAGE_ASSET_DATA
/* #include "new_badge_monsters_assets.h" */
#include "assetList.h"
#include "2024-badge-monsters/2024-badge-monsters.h"
#undef DEFINE_IMAGE_ASSET_DATA
#include "new_badge_monsters_ir.h"
#include "audio.h"
#include "badge.h"
#include "button.h"
#include "colors.h"
#include "framebuffer.h"
#include "key_value_storage.h"
#include "utils.h"

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#ifdef __linux__
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

/***************************************** TYPES *************************************************/
enum app_states {
    APP_INIT,
    GAME_MENU,
    MONSTER_MENU,
    SHOW_MONSTER,
    SHOW_DESCRIPTION,
    TRADE_MONSTERS,
    EXIT_APP
#ifdef __linux__
    , ENABLE_ALL_MONSTERS //7 for testing, grants ownership of all monsters
#endif
};

struct new_monster
{
    char name[DYNMENU_MAX_TITLE];
    int owned;
    int color;
    char *blurb;
    const struct asset2 *asset;
};

enum menu_level_t {
    GAME_MENU_LEVEL,
    MONSTER_MENU_LEVEL
};

// each game state is mapped to a function with this type signature
typedef void (*state_to_function_map_fn_type)(void);

/***************************************** PROTOTYPES ********************************************/
/********* STATE FUNCTIONS ****************************/
// code should not call these directly; instead set app_state
static void app_init(void);
static void game_menu(void);
static void monster_menu(void);
static void show_monster(void);
static void show_description(void);
static void trade_monsters(void);
static void exit_app(void);

/********* INPUT HANDLERS **************************/
static void game_menu_button_handler(void);
static void monster_menu_button_handler(void);
static void show_monster_button_handler(void);
static void show_description_button_handler(void);
static void trade_monsters_button_handler(void);

/******** SAVE & RESTORE ***************************/
char *key_from_monster(const struct new_monster *m, char *key, size_t len);
static void set_monster_owned(const int monster_id, const bool owned);
static void load_from_flash(void);
static void save_to_flash(void);

/******** ENABLE ***********************************/
// void enable_monster(const int monster_id);
#ifdef __linux__
static void enable_all_monsters(void);
#endif

/******** TRADING **********************************/
static void trade_monsters(void);

/******** MENUS ************************************/

static void change_menu_level(enum menu_level_t level);
static void show_monster_count(void);
static void setup_monster_menu(void);
static void setup_main_menu(void);

#ifdef __linux__
static const char *menu_level_str(const enum menu_level_t menu_level);
#endif


/***************************************** GLOBALS ***********************************************/
struct new_monster new_monsters[] = {
    {
        "Vorlith",
        true,
        RED,
        "A large, amphi-\nbious monster\nwith a biolumi-\nnescent under\nbelly, capable\n"
	"of emitting\nelectrical dis-\ncharges under\nwater, in\nhabiting the\noceans of dis-\ntant moons.",
        &bm1
    }
  , {
        "Nekroth",
        false,
        RED,
        "A skeletal,\nundead-like\nmonster that\ndrains life\nforce from its\nvictims, "
	"leav-\ning them as\nwithered husks",
        &bm2
    }
  , {
        "Xarnok",
        false,
        RED,
        "A heavily arm-\nored beast with\nmultiple eyes\nand sharp man-\ndibles, known\n"
	"for its brute\nstrength and\nability to\ntunnel through\nsolid rock on\nasteroid belt",
        &bm3
    }
  , {
        "Obsidrax",
        false,
        RED,
        "A hulking beast\nwith obsidian\nskin and the\nability to\nmanipulate\nmagma, often\nfound in volca-\nnic regions.",
        &bm4
    }
  , {
        "Chronor",
        false,
        RED,
        "A time manipu-\nlating entity\nthat can slow\ndown or speed\nup time around\nit, using this\n"
	"ability to\nevade attacks\nor catch prey.",
        &bm5
    }
  , {
        "Lumorax",
        false,
        RED,
        "A biolumine-\nscent predator\nthat uses light\nto lure prey.",
        &bm6
    }
  , {
        "Phantax",
        false,
        RED,
        "An ethereal\nbeing composed\nof dark matter,\nable to pass\nthrough\n"
	"physical ob-\njects and mani-\npulate shadows.",
        &bm7
    }
  , {
        "Thragor",
        false,
        RED,
        "A massive, bi-\npedal monster\nwith volcanic\nrock skin and\nthe ability to\n"
	"spew molten\nlava from its\nmouth, dwelling\nnear the cores\nof unstable\nplanets.",
        &bm8
    }
  , {
        "Glacira",
        false,
        RED,
        "An ice-covered\npredator with a\nchilling breath\nthat can freeze\n"
	"anything in its\npath, thriving\nin icy environ-\nments.",
        &bm9
    }
  , {
        "Quasarix",
        false,
        RED,
        "A cosmic entity\nthat emits pow-\nerful radiation\nbursts, disin-\ntegrating any-\n"
	"thing within\nits reach, and\nliving in deep\nspace.",
        &bm10
    }
  , {
        "Pirate",
        false,
        RED,
        "TBD",
        &bm11
    }
  , {
        "Blitzar",
        false,
        RED,
        "A lightning-\nfast quadruped\nwith electric\nblue fur that\ncan generate\n"
	"powerful static\ndischarges to\nstun its prey.",
        &bm12
    }
  , {
        "Lunaraq",
        false,
        RED,
        "A giant six-\nlegged predator\nwith a skull\nlike head and\nrazor-sharp\n"
	"claws, using\nits speed and\nagility to hunt\nin low-gravity\nenvirnoments",
        &bm13
    }
  , {
        "Flarix",
        false,
        RED,
        "A two headed\nfire-breathing\nreptilian beast\nwith scales\nthat can with-\n"
	"stand extreme\nheat, dwelling\nnear volcanic\nvents on molten\nplanets.",
        &bm14
    }
  , {
        "Spirox",
        false,
        RED,
        "A spore-based\norganism that\ninfects and\ntakes control\nof other crea-\n"
	"tures, turning\nthem into zombi-\nfied versions\nof themselves",
        &bm15
    }
  , {
        "Skorith",
        false,
        RED,
        "A predatory\ninsectoid with\na metallic exo-\nskeleton and\nacidic saliva,\n"
	"capable of\nmelting through\nmetal to reach\nits prey.",
        &bm16
    }
  , {
        "Eclipsar",
        false,
        RED,
        "A shadowy wing-\ned creature\nthat thrives in\nthe darkness,\ncapable of\ncreating areas\n"
	"of absolute\ndarkness to\nambush its\nprey.",
        &bm17
    }
  , {
        "Nyralith",
        false,
        RED,
        "A crystalline,\nspider-like\nmonster that\ncan generate\npowerful energy\n"
	"beams from its\neye, inhabiting\nthe caves of\nmineral-rich\nasteroids.",
        &bm18
    }
  , {
        "Zephyra",
        false,
        RED,
        "A delicate,\nbutterfly-like\ncreature that\nflits through\nthe air with\nease, using its\n"
	"wings to create\nmesmerizing\nlight patterns\nto attract prey.",
        &bm19
    }
  , {
        "Mythra",
        false,
        RED,
        "A ghostly, spec-\ntral entity\nthat feeds on\nthe psychic\nenergy of sen-\n"
	"tient beings,\nable to phase\nin and out of\nreality at\nwill.",
        &bm20
    }
  , {
        "Raven",
        false,
        RED,
        "TBD",
        &bm20
    }
};

// type, but it must be declared AFTER new_monsters for ARRAYSIZE
/*
 * global state for game -- used by almost all functions.
*/
struct game_state {
    // skip redrawing screen if no changes
    bool screen_changed;
    unsigned int current_monster; // active monster
    unsigned int nmonsters; // total monsters collectible
    enum app_states app_state;
    // TODO: do we still need this?
    bool trading_monsters_enabled;
    // user starts with a single monster, chosen based on badge ID
    unsigned int initial_mon;
    // menu used for both game and monster menu, rebuilt when menu changed
    struct dynmenu menu;
    // space for (longer) monster menu + 1 extra menu item: "EXIT"
    struct dynmenu_item menu_item[ARRAYSIZE(new_monsters) + 1];
    // TODO: since the two menus are different app states, we might not need this
    enum menu_level_t menu_level;
};


static struct game_state state = {
    .screen_changed = false,
    .current_monster = 0,
    .nmonsters = 1, // user starts w/one
    .app_state = APP_INIT,
    .trading_monsters_enabled = false,
    .initial_mon = 0, // set later from badge id
    .menu = {
        .title = "Badge Monsters",
        .title2 = "",
        .title3 = "",
        .item = NULL,
        .nitems = 0,
        .max_items = 8,
        .current_item = 0,
        .menu_active = 0,
        .chosen_cookie = 0,
        .color = 0xFFFFFF,
        .selected_color = 0x0,
	.selection_made = DYNMENU_SELECTION_VOID,
    },
    .menu_item = {},
    .menu_level = GAME_MENU_LEVEL
};

static state_to_function_map_fn_type state_to_function_map[] = {
    app_init,
    game_menu,
    monster_menu,
    show_monster,
    show_description,
    trade_monsters,
    exit_app,
#ifdef __linux__
    enable_all_monsters,
#endif
};

/***************************************** End of GLOBALS ****************************************/

void badge_monsters_cb(__attribute__((unused)) struct menu_t *m)
{
    state_to_function_map[state.app_state]();
}

/***************************************** STATE FUNCTIONS ***************************************/
/*
 * Each of these functions is called directly from the callback based on the current app state.
 * Each should do roughly the following:
 * 1. If screen_changed is false, draw to the screen based on the current state.
 * 2. check for user input
 * 3. set app_state to next state, if it's different
 */

/*
 * Initializes data structures and screen, loads any saved monsters, sets up main menu.
 * state --> GAME_MENU
 */
static void app_init()
{
    // graphics hardware init
    FbInit();
    // set up to receive IR
    register_ir_packet_callback(ir_packet_callback);
    // initialize menu and load main menu
    dynmenu_init(&state.menu, state.menu_item, ARRAYSIZE(state.menu_item));
    setup_main_menu();
    state.menu_level = GAME_MENU_LEVEL;

    // set up monster data structures, load
    state.nmonsters = ARRAYSIZE(new_monsters);
    load_from_flash();
    // initial monster different for different badges
    state.initial_mon = badge_system_data()->badgeId % state.nmonsters;
    state.current_monster = state.initial_mon;
    enable_monster(state.initial_mon);

    // tell app to draw screen, go to menu
    state.screen_changed = true;
    state.app_state = GAME_MENU;
    LOG("app_init complete\n");
}

/*
 * Saves monsters, de-initializes data, return to badge menu
 */
static void exit_app(void)
{
    state.app_state = APP_INIT;
    save_to_flash();
    unregister_ir_packet_callback(ir_packet_callback);
    returnToMenus();
}

static void game_menu(void)
{
    if (state.menu_level != GAME_MENU_LEVEL) {
        state.menu_level = GAME_MENU_LEVEL;
        change_menu_level(GAME_MENU_LEVEL);
    }
    if (state.screen_changed) {
        LOG("game_menu(): draw_menu()\n");
        dynmenu_draw(&state.menu);
        FbPushBuffer();
        state.screen_changed = false;
    }
    check_for_incoming_packets();
    game_menu_button_handler();
}

static void monster_menu(void)
{
    if (state.menu_level != MONSTER_MENU_LEVEL) {
        state.menu_level = MONSTER_MENU_LEVEL;
        change_menu_level(MONSTER_MENU_LEVEL);
    }
    if (state.screen_changed) {
        LOG("monster_menu(): draw_menu()\n");
        dynmenu_draw(&state.menu);
        show_monster_count();
        FbPushBuffer();
        LOG("monster_menu(): FbPushBuffer()\n");
        state.screen_changed = false;
    }
    check_for_incoming_packets();
    monster_menu_button_handler();
}

/*
 * If trading_monstes_enabled == false, sets it to true and draws a message to screen.
 * Sends an IR packet every 10 loops.
 */
static void trade_monsters(void)
{
    static int counter = 0;

    counter++;
    if ((counter % 10) == 0) { /* transmit our monster IR packet */
        build_and_send_packet(BADGE_IR_GAME_ADDRESS, BADGE_IR_BROADCAST_ID,
                              (OPCODE_XMIT_MONSTER << 12) | (state.initial_mon & 0x01ff));
        audio_out_beep(500, 100);
    }
    if (!state.trading_monsters_enabled) {
        FbClear();
        FbMove(10, 60);
        FbWriteLine("TRADING");
        FbMove(10, 70);
        FbWriteLine("MONSTERS!");
        FbPushBuffer();
        state.trading_monsters_enabled = true;
    }
    trade_monsters_button_handler();
}


/*
 * Displays state.current_monster on the screen.
 */
static void show_monster(void)
{
    const struct new_monster *monster = &new_monsters[state.current_monster];

    if (state.screen_changed) {
        LOG("render_monster: %s\n", monster->name);
        FbClear();
        FbMove(0, 10);
        FbImage4bit2(monster->asset, 0);
        if(state.current_monster == state.initial_mon)
        {
            FbMove(0,10);
            FbWriteLine("--starting-mon--");
        }
        // TODO: approximately center based on name length
        FbMove(10,0);
        FbColor(monster->color);
        FbWriteLine(monster->name);

        FbColor(WHITE);
        FbMove(43, LCD_YSIZE - 15);
        FbWriteLine("|down|");
        FbColor(GREEN);

        FbMove(5, LCD_YSIZE - 15);
        FbWriteLine("<Back");


        FbMove(90, LCD_YSIZE - 15);
        FbWriteLine("desc>");

        FbSwapBuffers();
        FbPushBuffer();
        state.screen_changed = false;
    }
    show_monster_button_handler();
}

static void show_description(void)
{
    const char *desc = new_monsters[state.current_monster].blurb;
    if (state.screen_changed) {
        FbClear();
        FbColor(WHITE);

        FbMove(8, 5);
        FbWriteString(desc);
        FbMove(5, 120);
        FbWriteLine("<Back");
        FbPushBuffer();
    }
    state.screen_changed = false;
    show_description_button_handler();
}


/***************************************** INPUT HANDLERS ****************************************/
// app_init() and exit_app() don't need handlers

/*
 * Handles input for the main game menu:
 *
 * "up" --> go to previous menu item
 * "down" --> go to next menu item
 * "left"/"right" --> do nothing
 * "click" --> go to app state associated with menu item
 */
static void game_menu_button_handler(void) {
    const int down_latches = button_down_latches();
#if BADGE_HAS_ROTARY_SWITCHES
    const int rotary = button_get_rotation(0);
#define ROTATION_POS(x) ((x) > 0)
#define ROTATION_NEG(x) ((x) < 0)
#else
#define ROTATION_POS(x) 0
#define ROTATION_NEG(x) 0
#endif

    if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || ROTATION_NEG(rotary))
    {
        dynmenu_change_current_selection(&state.menu, -1);
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || ROTATION_POS(rotary))
    {
        dynmenu_change_current_selection(&state.menu, 1);
        state.screen_changed = true;
    }
    else if (
#if BADGE_HAS_ROTARY_SWITCHES
	BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
#endif
	    BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
    {
        const struct dynmenu_item menu_item = state.menu.item[state.menu.current_item];
        state.app_state = menu_item.next_state;
        state.screen_changed = true;
    }
#if BADGE_HAS_ROTARY_SWITCHES
    else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches)) {
        state.app_state = EXIT_APP;
    }
#endif
}

/*
 * Handles input for the monster menu.
 *
 * "up" --> go to previous monster (and update current_monster)
 * "down" --> go to next monster (and update current_monster)
 * "left" --> change to MAIN_MENU level
 * "right"/"click" --> display currently selected monster
 */
static void monster_menu_button_handler(void) {
    const int down_latches = button_down_latches();
#if BADGE_HAS_ROTARY_SWITCHES
    const int rotary = button_get_rotation(0);
#endif

    if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || ROTATION_NEG(rotary))
    {
        dynmenu_change_current_selection(&state.menu, -1);
        state.current_monster = state.menu.item[state.menu.current_item].cookie;
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || ROTATION_POS(rotary))
    {
        dynmenu_change_current_selection(&state.menu, 1);
        state.current_monster = state.menu.item[state.menu.current_item].cookie;
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
             BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)
#if BADGE_HAS_ROTARY_SWITCHES
		|| BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches)
#endif
	) {
        state.app_state = GAME_MENU;
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
#if BADGE_HAS_ROTARY_SWITCHES
             BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
#endif
             BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
    {
        state.app_state = state.menu.item[state.menu.current_item].next_state;
        state.screen_changed = true;
    }
}

/*
 * On any button click, ends trading, returns to game menu
 */
static void trade_monsters_button_handler(void) {
    const int down_latches = button_down_latches();

    if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
#if BADGE_HAS_ROTARY_SWITCHES
        BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
#endif
        BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
        state.trading_monsters_enabled = false;
        state.app_state = GAME_MENU;
        state.screen_changed = true;
    }
}

/*
 * Handles input for monster description screen
 *
 * "up" -->
 * "down" -->
 * "left" --> redisplay current monster
 * "right"/"click" -->
 */
static void show_description_button_handler(void) {
    const int down_latches = button_down_latches();

    if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
             BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
    {
        state.app_state = SHOW_MONSTER;
        state.screen_changed = true;
    }
}

/*
 * Handles input for show monster screen
 *
 * "up" --> display previous monster
 * "down" --> display next monster
 * "left" --> back
 * "right"/"click" -->
 */
static void show_monster_button_handler(void) {
    const int down_latches = button_down_latches();
#if BADGE_HAS_ROTARY_SWITCHES
    const int rotary = button_get_rotation(0);
#endif

    if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || ROTATION_NEG(rotary))
    {
        // we change position in menu, but we keep displaying monster instead
        dynmenu_change_current_selection(&state.menu, -1);
        // skip "EXIT" item
        // Assumes at least one monster in the menu!!
        while (state.menu.item[state.menu.current_item].next_state != SHOW_MONSTER) {
            dynmenu_change_current_selection(&state.menu, -1);
        }
        state.current_monster = state.menu.item[state.menu.current_item].cookie;
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || ROTATION_POS(rotary))
    {
        dynmenu_change_current_selection(&state.menu, 1);
        while (state.menu.item[state.menu.current_item].next_state != SHOW_MONSTER) {
            dynmenu_change_current_selection(&state.menu, 1);
        }
        state.current_monster = state.menu.item[state.menu.current_item].cookie;
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
             BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
    {
        state.app_state = MONSTER_MENU;
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
#if BADGE_HAS_ROTARY_SWITCHES
             BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
#endif
             BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
    {
        state.app_state = SHOW_DESCRIPTION;
        state.screen_changed = true;
    }
}

/***************************************** SAVE/RESTORE ******************************************/

char *key_from_monster(const struct new_monster *m, char *key, size_t len)
{
    snprintf(key, len, "monster/%s", m->name);

    return key;
}

static void load_from_flash()
{
    char key[20];
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
	flash_kv_get_int(key_from_monster(m, key, sizeof(key)), &m->owned);
    }
}

static void save_if_enabled(const struct new_monster *m)
{
    char key[20];

    if (m->owned) {
        flash_kv_store_int(key_from_monster(m, key, sizeof(key)), m->owned);
    }
}

static void save_to_flash(void)
{
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
        save_if_enabled(m);
    }
}

/***************************************** MONSTER STATUS ****************************************/
static void set_monster_owned(const int monster_id, const bool owned)
{
    new_monsters[monster_id].owned = owned;
}

/*
 * Marks the monster with id `monster_id` as owned by user, beeps.
 */
void enable_monster(int monster_id)
{
    if(monster_id < 0 || (size_t) monster_id > ARRAYSIZE(new_monsters) - 1) {
        LOG("enable_monster: index %d is out of bounds!\n", monster_id);
        return;
    }
    set_monster_owned(monster_id, true);
    audio_out_beep(900, 200);
    LOG("enabled monster: %d\n", monster_id);
}

#ifdef __linux__
static void enable_all_monsters(void)
{
    for (size_t i = 0; i < ARRAYSIZE(new_monsters); i++)
        enable_monster(i);
    FbClear();
    FbMove(2, 2);
    FbWriteString("ALL MONSTERS\nENABLED!");
    state.app_state = GAME_MENU;
}
#endif


/***************************************** MENUS *************************************************/

/*
 * Adds a line to the current screen showing # of monsters owned / total # of monsters.
 */
static void show_monster_count(void)
{
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

    snprintf( available_monsters, sizeof(available_monsters), "%d", (int)ARRAYSIZE(new_monsters));
    snprintf( unlocked_monsters, sizeof(unlocked_monsters), "%d", nunlocked);

    FbMove(10,25);
    FbWriteLine("Collected:");
    FbWriteLine(unlocked_monsters);
    FbWriteLine("/");
    FbWriteLine(available_monsters);
}

/*
 * Changes the menu from the current level to `level`, runs setup function.
 *
 * state changes:
 * state.screen_changed -> true
 * state.menu_level = level
 * state.menu.current_item (conditional)
 * state.current_monster (conditional)
 */
static void change_menu_level(enum menu_level_t level)
{
    if (level == GAME_MENU_LEVEL) {
        setup_main_menu();
    } else {
        setup_monster_menu();
        // set current monster to same as previous, if it's in menu
        if (state.menu.current_item > state.menu.nitems - 1)
            state.menu.current_item = state.menu.nitems - 1;
    }
    state.menu_level = level;
    LOG("change_menu_level: new level = %s\n", menu_level_str(state.menu_level));
    state.screen_changed = true;
}


#ifdef __linux__
static const char *menu_level_str(const enum menu_level_t menu_level) {
    // TODO: convert all "main" references to "game"
    return menu_level == GAME_MENU_LEVEL ? "GAME_MENU_LEVEL" : "MONSTER_MENU_LEVEL";
}
#endif

/*
 * Builds a monster menu by clearing the menu, setting the title to "Monsters", and
 * for each monster which is owned, adding an entry with the monster's name, the next
 * state SHOW_MONSTER, and a cookie set to the index of the monster in the new_monsters
 * array.
 * If the current monster is shown in the menu (?), set the current item to the current
 * monster, otherwise set the current monster to the current menu item.
 */
static void setup_monster_menu(void)
{
    dynmenu_clear(&state.menu);
    state.menu.menu_active = 0;
    strncpy(state.menu.title, "Monsters", sizeof(state.menu.title));

    int index = 0;
    for(const struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++){
        if(m->owned) {
            dynmenu_add_item(&state.menu, (char *)m->name, SHOW_MONSTER, index);
            LOG("Added monster %d to menu\n", index);
        }
        index++;
    }
    dynmenu_add_item(&state.menu, "EXIT", GAME_MENU, 0);
    // I'm not sure this is accomplishing what it's supposed to
    if (state.current_monster < state.menu.max_items)
        state.menu.current_item = state.current_monster;
    else
        state.current_monster = state.menu.current_item;
    state.screen_changed = true;
}

/*
 * Builds the game main menu. Note that this and monster menu share the same menu
 * structure, it gets rebuilt when the menu_level changes.
 */
static void setup_main_menu(void)
{
    dynmenu_clear(&state.menu);
    snprintf(state.menu.title, sizeof(state.menu.title), "Badge Monsters");
    dynmenu_add_item(&state.menu, "Monsters", MONSTER_MENU, 0);
    dynmenu_add_item(&state.menu, "Trade Monsters", TRADE_MONSTERS, 1);
    dynmenu_add_item(&state.menu, "EXIT", EXIT_APP, 2);
#ifdef __linux__
    /* For testing purposes allow enabling all monsters on linux */
    dynmenu_add_item(&state.menu, "Test Monsters", ENABLE_ALL_MONSTERS, 3);
#endif
    state.screen_changed = true;
}

//**************************** DISPLAY ***********************************************

/*
 * Displays a monster, showing a new one each time it is called. For use by screen
 * saver.
 */
void render_screen_save_monsters(void) {
    static unsigned char current_index = 0;
    current_index++;
    // wrap-around if out of bounds
    current_index %= ARRAYSIZE(new_monsters) - 1;

    FbClear();
    FbColor(BLACK);
    FbImage4bit2(new_monsters[current_index].asset, 0);
}

