/*********************************************

 Pseudo 3D maze game for RVASEC badge hardware.

 Author: Stephen M. Cameron <stephenmcameron@gmail.com>
 (c) 2019 Stephen M. Cameron

 If you're wondering why this program is written
 in such a strange way, it's because of the environment
 it needs to fit into.  This program is meant to run
 on a PIC32 in a kind of cooperative multiprogram system
 and can't monopolize the CPU for any significant length
 of time (though it does have some compatibility bodges that
 allow it to run as a native linux program too.)  That is
 why maze_cb() is just a big switch statement that does
 different things based on maze_program_state and why there
 are so many global variables.  So the weirdness is there
 for a reason, and is not how I would normally write a program.

**********************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "xorshift.h"
#include "achievements.h"
#include "dynmenu.h"

#define UNUSED __attribute__((unused))


/* Program states.  Initial state is MAZE_GAME_INIT */
enum maze_program_state_t {
    MAZE_GAME_INIT = 0,
    MAZE_GAME_START_MENU,
    MAZE_LEVEL_INIT,
    MAZE_BUILD,
    MAZE_PRINT,
    MAZE_RENDER,
    MAZE_OBJECT_RENDER,
    MAZE_RENDER_ENCOUNTER,
    MAZE_DRAW_STATS,
    MAZE_SCREEN_RENDER,
    MAZE_PROCESS_COMMANDS,
    MAZE_DRAW_MAP,
    MAZE_DRAW_MENU,
    MAZE_STATE_GO_DOWN,
    MAZE_STATE_GO_UP,
    MAZE_STATE_FIGHT,
    MAZE_STATE_FLEE,
    MAZE_RENDER_COMBAT,
    MAZE_COMBAT_MONSTER_MOVE,
    MAZE_STATE_PLAYER_DEFEATS_MONSTER,
    MAZE_STATE_PLAYER_DIED,
    MAZE_CHOOSE_POTION,
    MAZE_QUAFF_POTION,
    MAZE_CHOOSE_WEAPON,
    MAZE_WIELD_WEAPON,
    MAZE_CHOOSE_ARMOR,
    MAZE_DON_ARMOR,
    MAZE_CHOOSE_TAKE_OBJECT,
    MAZE_CHOOSE_DROP_OBJECT,
    MAZE_TAKE_OBJECT,
    MAZE_DROP_OBJECT,
    MAZE_THROW_GRENADE,
    MAZE_WIN_CONDITION,
    MAZE_READ_SCROLL,
    MAZE_EXIT,
};
static enum maze_program_state_t maze_program_state = MAZE_GAME_INIT;
static int maze_back_wall_distance = 7;
static int maze_object_distance_limit = 0;
static int maze_start = 0;
static int maze_scale = 12;
static int current_drawing_object = 0;
static unsigned int xorshift_state = 0xa5a5a5a5;
static unsigned char combat_mode = 0;

#define TERMINATE_CHANCE 5
#define BRANCH_CHANCE 30
#define OBJECT_CHANCE 20

/* Dimensions of generated maze */
#define XDIM 24 /* Must be divisible by 8 */
#define YDIM 24
#define NLEVELS 3

static unsigned int maze_random_seed[NLEVELS] = { 0 };
static int maze_previous_level = -1;
static int maze_current_level = 0;
static int level_color[] = { YELLOW, CYAN, WHITE };
static char game_is_won = 0;

#define MAZE_PLACE_PLAYER_DO_NOT_MOVE 0
#define MAZE_PLACE_PLAYER_BENEATH_UP_LADDER 1
#define MAZE_PLACE_PLAYER_ABOVE_DOWN_LADDER 2
static int maze_player_initial_placement = MAZE_PLACE_PLAYER_BENEATH_UP_LADDER;

/* Array to hold the maze.  Each square of the maze is represented by 1 bit.
 * 0 means solid rock, 1 means empty passage.
 */
static unsigned char maze[XDIM >> 3][YDIM] = { { 0 }, };
static unsigned char maze_visited[XDIM >> 3][YDIM] = { { 0 }, };

/*
 * Stack structure used when generating maze to remember where we left off.
 */
static struct maze_gen_stack_element {
    unsigned char x, y, direction;
} maze_stack[50];
#define MAZE_STACK_EMPTY -1
static short maze_stack_ptr = MAZE_STACK_EMPTY;
static int maze_size = 0;
static int max_maze_stack_depth = 0;
static int generation_iterations = 0;

static struct player_state {
    unsigned char x, y, direction;
    unsigned char combatx, combaty;
    unsigned char hitpoints;
    unsigned char weapon;
    unsigned char armor;
    int gp;
} player, combatant;

static struct potion_descriptor {
    char *adjective;
    signed char health_impact;
} potion_type[] = {
    { "SMELLY", 0 },
    { "FUNKY", 0 },
    { "MISTY", 0 },
    { "FROTHY", 0 },
    { "FOAMY", 0 },
    { "FULMINATING", 0 },
    { "SMOKING", 0 },
    { "SPARKLING", 0 },
    { "BUBBLING", 0 },
    { "ACRID", 0 },
    { "PUNGENT", 0 },
    { "STINKY", 0 },
    { "AROMATIC", 0 },
    { "GELATINOUS", 0 },
    { "JIGGLY", 0 },
    { "GLOWING", 0 },
    { "LUMINESCENT", 0 },
    { "PEARLESCENT", 0 },
    { "FRUITY", 0 },
};

static struct weapon_descriptor {
    char *adjective;
    char *name;
    unsigned char ranged;
    unsigned char shots;
    unsigned char damage;
} weapon_type[] = {
 { "ORDINARY", "DAGGER", 0, 0, 3 },
 { "BLACK", "SWORD", 0, 0, 5 },
 { "SHORT", "SWORD", 0, 0, 8 },
 { "BROAD", "SWORD", 0, 0, 10 },
 { "LONG", "SWORD", 0, 0, 10 },
 { "DWARVEN", "SWORD", 0, 0, 12 },
 { "ORCISH", "SWORD", 0, 0, 13 },
 { "GOLDEN", "SWORD", 0, 0, 14 },
 { "ELVISH", "SWORD", 0, 0, 15 },
 { "LOST", "KATANA", 0, 0, 15 },
 { "GREAT", "SWORD", 0, 0, 16 },
 { "COSMIC", "SWORD", 0, 0, 20 },
 { "GLOWING", "SWORD", 0, 0, 22 },
 { "FIRE", "SABER", 0, 0, 25 },
 { "FLAMING", "SCIMITAR", 0, 0, 27 },
 { "ELECTRO", "CUTLASS", 0, 0, 30 },
};

static struct scroll_t {
	char text1[20];
	char text2[20];
	char text3[20];
} scroll[] = {
	{
		"MAD SCRIBBLINGS",
		"OF AN INSANE",
		"WIZARD:LASERTAG",
	},
	{
		"CRYPTICALLY",
		"IT SAYS,",
		"'XYZZY'",
	},
	{
		"MADNESS AWAITS",
		"WHO ENTERS!",
		"TURN BACK!",
	},
	{
		"BEWARE THE",
		"JABBERWOCK,",
		"MY SON!",
	},
	{
		"GIVE STRONG",
		"DRINK TO THOSE",
		"WHO ARE DYING",
	},
	{
		"WE CANNOT",
		"ESCAPE! THEY",
		"ARE COMING!",
	},
	{
		"READ NOT THE",
		"BOOK OF FACES!",
		"IT READS YOU!",
	},
	{
		"ROBERT MORRIS",
		"WAS HERE",
		"NOV. 1988",
	},

};
#define NSCROLLS (ARRAYSIZE(scroll))

static struct armor_descriptor {
    char *adjective;
    char *name;
    unsigned char protection;
} armor_type[] = {
    { "LEATHER", "ARMOR", 2 },
    { "CHAINMAIL", "ARMOR", 5 },
    { "PLATE", "ARMOR", 8 },
};

union maze_object_type_specific_data {
    struct {
        unsigned char hitpoints;
        unsigned char speed;
    } monster;
    struct {
        unsigned char type; /* index into potion_type[] */
    } potion;
    struct {
        unsigned char type; /* index into armor_type[] */
    } armor;
    struct {
      unsigned char gp;
    } treasure;
    struct {
      unsigned char type; /* index into weapon_type[] */
    } weapon;
};

enum maze_object_category {
    MAZE_OBJECT_MONSTER,
    MAZE_OBJECT_WEAPON,
    MAZE_OBJECT_SCROLL,
    MAZE_OBJECT_GRENADE,
    MAZE_OBJECT_KEY,
    MAZE_OBJECT_POTION,
    MAZE_OBJECT_ARMOR,
    MAZE_OBJECT_TREASURE,
    MAZE_OBJECT_DOWN_LADDER,
    MAZE_OBJECT_UP_LADDER,
    MAZE_OBJECT_CHALICE,
};

struct maze_object_template {
    char name[14];
    int color;
    enum maze_object_category category;
    const struct point *drawing;
    int npoints;
    short speed;
    unsigned char hitpoints;
    unsigned char damage;
};

struct maze_object {
    unsigned char x, y;
    unsigned char type;
    union maze_object_type_specific_data tsd;
};

static const struct point scroll_points[] =
#include "maze_drawings/scroll_points.h"

static const struct point dragon_points[] =
#include "maze_drawings/dragon_points.h"

static const struct point chest_points[] =
#include "maze_drawings/chest_points.h"

static const struct point cobra_points[] =
#include "maze_drawings/cobra_points.h"

static const struct point grenade_points[] =
#include "maze_drawings/grenade_points.h"

static const struct point orc_points[] =
#include "maze_drawings/orc_points.h"

static const struct point phantasm_points[] =
#include "maze_drawings/phantasm_points.h"

static const struct point potion_points[] =
#include "maze_drawings/potion_points.h"

static const struct point shield_points[] =
#include "maze_drawings/shield_points.h"

static const struct point sword_points[] =
#include "maze_drawings/sword_points.h"

static const struct point up_ladder_points[] =
#include "maze_drawings/up_ladder_points.h"

static const struct point down_ladder_points[] =
#include "maze_drawings/down_ladder_points.h"

static const struct point player_points[] =
#include "maze_drawings/player_points.h"

static const struct point bones_points[] =
#include "maze_drawings/bones_points.h"

static const struct point chalice_points[] =
#include "maze_drawings/chalice_points.h"

#define MAZE_NOBJECT_TYPES 14
static int nobject_types = MAZE_NOBJECT_TYPES;

#define MAX_MAZE_OBJECTS 30
#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

static struct maze_object_template maze_object_template[] = {
    { "SCROLL", BLUE, MAZE_OBJECT_SCROLL, scroll_points, ARRAYSIZE(scroll_points), 0, 0, 10 },
    { "DRAGON", GREEN, MAZE_OBJECT_MONSTER, dragon_points, ARRAYSIZE(dragon_points), 8, 40, 20 },
    { "CHEST", YELLOW, MAZE_OBJECT_TREASURE, chest_points, ARRAYSIZE(chest_points), 0, 0, 0 },
    { "COBRA", GREEN, MAZE_OBJECT_MONSTER, cobra_points, ARRAYSIZE(cobra_points), 2, 5, 10 },
    { "HOLY GRENADE", YELLOW, MAZE_OBJECT_GRENADE, grenade_points, ARRAYSIZE(grenade_points), 0, 0, 20 },
    { "SCARY ORC", GREEN, MAZE_OBJECT_MONSTER, orc_points, ARRAYSIZE(orc_points), 4, 15, 15 },
    { "PHANTASM", WHITE, MAZE_OBJECT_MONSTER, phantasm_points, ARRAYSIZE(phantasm_points), 6, 15, 4 },
    { "POTION", RED, MAZE_OBJECT_POTION, potion_points, ARRAYSIZE(potion_points), 0, 0, 30 },
    { "SHIELD", YELLOW, MAZE_OBJECT_ARMOR, shield_points, ARRAYSIZE(shield_points), 0, 0, 5 },
    { "SWORD", YELLOW, MAZE_OBJECT_WEAPON, sword_points, ARRAYSIZE(sword_points), 0, 0, 15 },
#define DOWN_LADDER 10
    { "LADDER", WHITE, MAZE_OBJECT_DOWN_LADDER, down_ladder_points, ARRAYSIZE(down_ladder_points), 0, 0, 0 },
#define UP_LADDER 11
    { "LADDER", WHITE, MAZE_OBJECT_UP_LADDER, up_ladder_points, ARRAYSIZE(down_ladder_points), 0, 0, 0 },
#define CHALICE 12
    { "CHALICE", YELLOW, MAZE_OBJECT_CHALICE, chalice_points, ARRAYSIZE(chalice_points), 0, 0, 0 },
};

struct dynmenu maze_menu;
struct dynmenu_item maze_menu_item[20];

static struct maze_object maze_object[MAX_MAZE_OBJECTS];
static int nmaze_objects = 0;

static int min_maze_size(void)
{
    return XDIM * YDIM / 3;
}

/* X and Y offsets for 8 cardinal directions: N, NE, E, SE, S, SW, W, NW */
static const char xoff[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const char yoff[] = { -1, -1, 0, 1, 1, 1, 0, -1 };

static void maze_stack_push(unsigned char x, unsigned char y, unsigned char direction)
{
    maze_stack_ptr++;
    if (maze_stack_ptr > 0 && (size_t) maze_stack_ptr >= (ARRAYSIZE(maze_stack))) {
        /* Oops, we blew our stack.  Start over */
#ifdef __linux__
        printf("Oops, stack blew... size = %d\n", maze_stack_ptr);
#endif
        maze_program_state = MAZE_LEVEL_INIT;
        maze_random_seed[maze_current_level] = xorshift(&xorshift_state);
        return;
    }
    if (max_maze_stack_depth < maze_stack_ptr)
        max_maze_stack_depth = maze_stack_ptr;
    maze_stack[maze_stack_ptr].x = x;
    maze_stack[maze_stack_ptr].y = y;
    maze_stack[maze_stack_ptr].direction = direction;
}

static void maze_stack_pop(void)
{
    maze_stack_ptr--;
}

static void player_init()
{
    player.hitpoints = 255;
    player.gp = 0;
    player.weapon = 255;
    player.armor = 255;
}

static void init_maze_objects(void)
{
    int i;

    if (maze_previous_level == -1) { /* game is just beginning */
        nmaze_objects = 0;
        memset(maze_object, 0, sizeof(maze_object));
        return;
    }
    /* zero out all objects not in player's possession */
    for (i = 0; i < MAX_MAZE_OBJECTS; i++)
        if (maze_object[i].x != 255)
            memset(&maze_object[i], 0, sizeof(maze_object[i]));
}

/* Initial program state to kick off maze generation */
static void maze_init(void)
{
    FbInit();
    xorshift_state = maze_random_seed[maze_current_level];
    if (xorshift_state == 0)
        xorshift_state = 0xa5a5a5a5;
    player.x = XDIM / 2;
    player.y = YDIM - 2;
    player.direction = 0;
    max_maze_stack_depth = 0;
    memset(maze, 0, sizeof(maze));
    memset(maze_visited, 0, sizeof(maze_visited));
    maze_stack_ptr = MAZE_STACK_EMPTY;
    maze_stack_push(player.x, player.y, player.direction);
    maze_program_state = MAZE_BUILD;
    maze_size = 0;
    init_maze_objects();
    combat_mode = 0;
    generation_iterations = 0;
}

/* Returns 1 if (x,y) is empty passage, 0 if solid rock */
static unsigned char is_passage(unsigned char x, unsigned char y)
{
    return maze[x >> 3][y] & (1 << (x % 8));
}

/* Sets maze square at x,y to 1 (empty passage) */
static void dig_maze_square(unsigned char x, unsigned char y)
{
    unsigned char bit = 1 << (x % 8);
    x = x >> 3;
    maze[x][y] |= bit;
    maze_size++;
}

static void mark_maze_square_visited(unsigned char x, unsigned char y)
{
    unsigned char bit = 1 << (x % 8);
    x = x >> 3;
    maze_visited[x][y] |= bit;
}

static int is_visited(unsigned char x, unsigned char y)
{
    return maze_visited[x >> 3][y] & (1 << (x % 8));
}

/* Returns 0 if x,y are in bounds of maze dimensions, 1 otherwise */
static int out_of_bounds(int x, int y)
{
    if (x < XDIM && y < YDIM && x >= 0 && y >= 0)
        return 0;
    return 1;
}

static unsigned char normalize_direction(int direction)
{
    if (direction < 0)
        return (unsigned char) direction + 8;
    if (direction > 7)
        return (unsigned char) direction - 8;
    return direction;
}

static unsigned char left_dir(int direction)
{
    return normalize_direction(direction - 2);
}

static unsigned char right_dir(int direction)
{
    return normalize_direction(direction + 2);
}

/* Consider whether digx, digy is diggable.  */
static int diggable(unsigned char digx, unsigned char digy, unsigned char direction)
{
    int i, startdir, enddir;

    if (out_of_bounds(digx, digy)) /* not diggable if out of bounds */
        return 0;


    startdir = left_dir(direction);
    enddir = normalize_direction(right_dir(direction) + 1);

    i = startdir;
    while (i != enddir) {
        unsigned char x, y;

        x = digx + xoff[i];
        y = digy + yoff[i];

        if (out_of_bounds(x, y)) /* We do not dig at the edge of the maze */
            return 0;
        if (is_passage(x, y)) /* do not connect to other passages */
            return 0;
        i = normalize_direction(i + 1);
    }
    return 1;
}

/* Rolls the dice and returns 1 chance percent of the time
 * e.g. random_chance(80) returns 1 80% of the time, and 0
 * 20% of the time
 */
static int random_choice(int chance)
{
    return (xorshift(&xorshift_state) % 10000) < (unsigned int) (100 * chance);
}

static int object_is_portable(int i)
{
    switch(maze_object_template[maze_object[i].type].category) {
    case MAZE_OBJECT_WEAPON:
    case MAZE_OBJECT_KEY:
    case MAZE_OBJECT_POTION:
    case MAZE_OBJECT_TREASURE:
    case MAZE_OBJECT_ARMOR:
    case MAZE_OBJECT_SCROLL:
    case MAZE_OBJECT_GRENADE:
    case MAZE_OBJECT_CHALICE:
        return 1;
    default:
        return 0;
    }
}

static int something_here(int x, int y)
{
    int i;

    for (i = 0; i < nmaze_objects; i++)
        if (maze_object[i].x == x && maze_object[i].y == y)
            return 1;
    return 0;
}

static void add_ladder(int ladder_type)
{
    int i, x, y;

    do {
        x = xorshift(&xorshift_state) % XDIM;
        y = xorshift(&xorshift_state) % YDIM;
    } while (!is_passage(x, y) || something_here(x, y));

    if (nmaze_objects < MAX_MAZE_OBJECTS - 1) {
        i = nmaze_objects;
    } else {
        for (i = 0; i < MAX_MAZE_OBJECTS; i++) {
            if (maze_object[i].x != 255 &&
                maze_object[i].type != DOWN_LADDER && maze_object[i].type != UP_LADDER)
                break;
        }
    }
    if (i >= MAX_MAZE_OBJECTS) { /* Player somehow has all objects */
        /* now what? */
    }

    maze_object[i].x = x;
    maze_object[i].y = y;
    maze_object[i].type = ladder_type;

    if ((maze_player_initial_placement == MAZE_PLACE_PLAYER_BENEATH_UP_LADDER &&
        ladder_type == UP_LADDER) ||
        (maze_player_initial_placement == MAZE_PLACE_PLAYER_ABOVE_DOWN_LADDER &&
        ladder_type == DOWN_LADDER)) {
        player.x = x;
        player.y = y;
    }

    if (i >= nmaze_objects - 1)
        nmaze_objects = i + 1;
}

static void add_ladders(int level)
{
    if (level < NLEVELS - 1)
        add_ladder(DOWN_LADDER);
    add_ladder(UP_LADDER);
}

static void add_chalice(int level)
{
    int i, x, y;
    if (level < NLEVELS - 1) {
#ifdef __linux__
        printf("Not adding chalice, level = %d < %d\n", level, NLEVELS - 1);
#endif
        return; /* chalice is only on deepest level */
    }

    do {
        x = xorshift(&xorshift_state) % XDIM;
        y = xorshift(&xorshift_state) % YDIM;
    } while (!is_passage(x, y) || something_here(x, y) || (x == player.x && y == player.y));

    if (nmaze_objects < MAX_MAZE_OBJECTS - 1) {
        i = nmaze_objects;
    } else {
        for (i = 0; i < MAX_MAZE_OBJECTS; i++) {
            if (maze_object[i].x != 255 &&
                maze_object[i].type != DOWN_LADDER && maze_object[i].type != UP_LADDER)
                break;
        }
    }
    if (i >= MAX_MAZE_OBJECTS) { /* Player somehow has all objects */
#ifdef __linux__
	printf("Didn't add chalice.\n");
#endif
        return; /* now what? */
    }

    maze_object[i].x = x;
    maze_object[i].y = y;
    maze_object[i].type = CHALICE;
#ifdef __linux__
	printf("Added chalice, object %d at %d, %d, level %d\n", i, x, y, level);
#endif
    if (i >= nmaze_objects - 1) {
        nmaze_objects = i + 1;
#ifdef __linux__
	printf("Added new object for chalice\n");
#endif
    }
}

static void add_random_object(int x, int y)
{
    int otype, i;

    /* Find a free object */
    for (i = 0; i < MAX_MAZE_OBJECTS; i++)
       if (maze_object[i].x == 0)
          break;
    if (i >= MAX_MAZE_OBJECTS)
        return;

    maze_object[i].x = x;
    maze_object[i].y = y;
    otype = xorshift(&xorshift_state) % (nobject_types - 3); /* minus 3 to exclude ladders and chalice */
    maze_object[i].type = otype;
    switch(maze_object_template[maze_object[i].type].category) {
    case MAZE_OBJECT_MONSTER:
        maze_object[i].tsd.monster.hitpoints =
            maze_object_template[otype].hitpoints + (xorshift(&xorshift_state) % 5);
        maze_object[i].tsd.monster.speed =
            maze_object_template[maze_object[i].type].speed;
        break;
    case MAZE_OBJECT_POTION:
        maze_object[i].tsd.potion.type = (xorshift(&xorshift_state) % ARRAYSIZE(potion_type));
        break;
    case MAZE_OBJECT_ARMOR:
        maze_object[i].tsd.armor.type = (xorshift(&xorshift_state) % ARRAYSIZE(armor_type));
        break;
    case MAZE_OBJECT_TREASURE:
        maze_object[i].tsd.treasure.gp = (xorshift(&xorshift_state) % 40);
        break;
    case MAZE_OBJECT_WEAPON:
        maze_object[i].tsd.weapon.type = (xorshift(&xorshift_state) % ARRAYSIZE(weapon_type));
        break;
    case MAZE_OBJECT_SCROLL:
    case MAZE_OBJECT_GRENADE:
    default:
        break;
    }
    if (i > nmaze_objects - 1)
        nmaze_objects = i + 1;
}

static void print_maze()
{
#ifdef __linux__
    int i, j;

    for (j = 0; j < YDIM; j++) {
        for (i = 0; i < XDIM; i++) {
            if (is_passage(i, j))
                if (j == player.y && i == player.x)
                    printf("@");
                else
                    printf(" ");
            else
                printf("#");
        }
        printf("\n");
    }
    printf("maze_size = %d, max stack depth = %d, generation_iterations = %d\n", maze_size, max_maze_stack_depth, generation_iterations);
#endif
    maze_program_state = MAZE_RENDER;
}

/* Normally this would be recursive, but instead we use an explicit stack
 * to enable this to yield and then restart as needed.
 */
static void generate_maze(void)
{
    unsigned char *x, *y, *d;
    unsigned char nx, ny;

    //BUILD_ASSERT((XDIM % 8) == 0); /* This is a build-time assertion that generates no code */

    x = &maze_stack[maze_stack_ptr].x;
    y = &maze_stack[maze_stack_ptr].y;
    d = &maze_stack[maze_stack_ptr].direction;

    generation_iterations++;

    dig_maze_square(*x, *y);

    if (random_choice(OBJECT_CHANCE))
        add_random_object(*x, *y);
    nx = *x + xoff[*d];
    ny = *y + yoff[*d];
    if (!diggable(nx, ny, *d))
        maze_stack_pop();
    if (maze_stack_ptr == MAZE_STACK_EMPTY) {
        maze_program_state = MAZE_PRINT;
        if (maze_size < min_maze_size()) {
#ifdef __linux
            printf("maze too small, starting over\n");
#endif
            maze_program_state = MAZE_LEVEL_INIT;
            maze_random_seed[maze_current_level] = xorshift(&xorshift_state);
        } else {
            add_ladders(maze_current_level);
            add_chalice(maze_current_level);
        }
        return;
    }
    *x = nx;
    *y = ny;
    if (random_choice(TERMINATE_CHANCE)) {
        maze_stack_pop();
        if (maze_stack_ptr == MAZE_STACK_EMPTY) {
            maze_program_state = MAZE_PRINT;
            if (maze_size < min_maze_size()) {
#ifdef __linux
                printf("maze too small, starting over\n");
#endif
                maze_program_state = MAZE_LEVEL_INIT;
                maze_random_seed[maze_current_level] = xorshift(&xorshift_state);
            } else {
                add_ladders(maze_current_level);
                add_chalice(maze_current_level);
            }
            return;
        }
    }
    if (random_choice(BRANCH_CHANCE)) {
        int new_dir = random_choice(50) ? right_dir(*d) : left_dir(*d);
        if (diggable(xoff[new_dir] + nx, yoff[new_dir] + ny, new_dir))
            maze_stack_push(nx, ny, (unsigned char) new_dir);
    }
}

static void draw_map()
{
    int x, y;

    FbClear();
    FbColor(GREEN);
    for (x = 0; x < XDIM; x++) {
        for (y = 0; y < YDIM; y++) {
            if (x == player.x && y == player.y) {
                FbColor(WHITE);
                FbLine(x * 3 - 2, y * 3 - 2, x * 3 + 2, y * 3 - 2);
                FbLine(x * 3 - 2, y * 3 - 1, x * 3 + 2, y * 3 - 1);
                FbLine(x * 3 - 2, y * 3 - 0, x * 3 + 2, y * 3 - 0);
                FbLine(x * 3 - 2, y * 3 + 1, x * 3 + 2, y * 3 + 1);
                FbLine(x * 3 - 2, y * 3 + 2, x * 3 + 2, y * 3 + 2);
                FbColor(GREEN);
                continue;
            }
            if (is_visited(x, y)) {
                FbHorizontalLine(x * 3 - 1, y * 3 - 1, x * 3 + 1, y * 3 - 1);
                FbHorizontalLine(x * 3 - 1, y * 3, x * 3 + 1, y * 3);
                FbHorizontalLine(x * 3 - 1, y * 3 + 1, x * 3 + 1, y * 3 + 1);
            }
       }
    }
    maze_program_state = MAZE_SCREEN_RENDER;
}

/* These integer ratios approximate 0.4, 0.4 * 0.8, 0.4 * 0.8^2, 0.4 * 0.8^3, 0.4 * 0.8^4, ...
 * which are used to approximate appropriate perspective scaling (reduction in size as viewing
 * distance linearly increases stepwise) while avoiding floating point operations.
 *
 * The denominator is always 1024.
 */
static const int drawing_scale_numerator[] = { 410, 328, 262, 210, 168, 134, 107, 86 };

void draw_object(const struct point drawing[], int npoints, int scale_index, int color, int x, int y)
{
    int i;
    int xcenter = x;
    int ycenter = y;
    int num;

    num = drawing_scale_numerator[scale_index];

    FbColor(color);
    for (i = 0; i < npoints - 1;) {
        if (drawing[i].x == -128) {
            i++;
            continue;
        }
        if (drawing[i + 1].x == -128) {
            i+=2;
            continue;
        }
        FbLine(xcenter + ((drawing[i].x * num) >> 10), ycenter + ((drawing[i].y * num) >> 10),
            xcenter + ((drawing[i + 1].x * num) >> 10), ycenter + ((drawing[i + 1].y * num) >> 10));
        i++;
    }
}

static void draw_left_passage(int start, int scale)
{
    FbVerticalLine(start, start, start, LCD_YSIZE - 1 - start);
    FbVerticalLine(start + scale, start + scale, start + scale, LCD_YSIZE - 1 - (start + scale));
    FbHorizontalLine(start, start + scale, start + scale, start + scale);
    FbHorizontalLine(start, LCD_YSIZE - 1 - (start + scale), start + scale, LCD_YSIZE - 1 - (start + scale));
}

static void draw_right_passage(int start, int scale)
{
    FbVerticalLine(LCD_XSIZE - 1 - start, start, LCD_XSIZE - 1 - start, LCD_YSIZE - 1 - start);
    FbVerticalLine(LCD_XSIZE - 1 - start - scale, start + scale, LCD_XSIZE - 1 - start - scale, LCD_YSIZE - 1 - start - scale);
    FbHorizontalLine(LCD_XSIZE - 1 - (start + scale), start + scale, LCD_XSIZE - 1 - start, start + scale);
    FbHorizontalLine(LCD_XSIZE - 1 - (start + scale), LCD_YSIZE - 1 - (start + scale), LCD_XSIZE - 1 - start, LCD_YSIZE - 1 - (start + scale));
}

static void draw_left_wall(int start, int scale)
{
    FbLine(start, start, start + scale, start + scale);
    FbLine(start, LCD_YSIZE - 1 - start, start + scale, LCD_YSIZE - 1 - (start + scale));
}

static void draw_right_wall(int start, int scale)
{
    FbLine(LCD_XSIZE - 1 - start, start, LCD_XSIZE - 1 - (start + scale), start + scale);
    FbLine(LCD_XSIZE - 1 - start, LCD_YSIZE - 1 - start, LCD_XSIZE - 1 - (start + scale), LCD_YSIZE - 1 - (start + scale));
}

static void draw_forward_wall(int start, UNUSED int scale)
{
    FbVerticalLine(start, start, start, LCD_YSIZE - 1 - start);
    FbVerticalLine(LCD_XSIZE - 1 - start, start, LCD_XSIZE - 1 - start, LCD_YSIZE - 1 - start);
    FbHorizontalLine(start, start, LCD_XSIZE - 1 - start, start);
    FbHorizontalLine(start, LCD_YSIZE - 1 - start, LCD_XSIZE - 1 - start, LCD_YSIZE - 1 - start);
}

static char *encounter_text = "x";
static char *encounter_adjective = "";
static char *encounter_name = "";
static unsigned char encounter_object = 255;

/* Returns 1 if traversal of ladder successful.
 * Returns 0 if you cannot go that way.
 */
static int go_up_or_down(int direction)
{
    int i, ok, ladder_type, placement;
    unsigned char has_chalice = 0;

    if (direction > 0) {
        ladder_type = DOWN_LADDER;
        placement = MAZE_PLACE_PLAYER_BENEATH_UP_LADDER;
    } else {
        ladder_type = UP_LADDER;
        placement = MAZE_PLACE_PLAYER_ABOVE_DOWN_LADDER;
    }

    ok = 0;
    /* Check if we are facing a down ladder */
    for (i = 0; i < nmaze_objects; i++) {
        if (maze_object[i].type == ladder_type &&
            maze_object[i].x == player.x && maze_object[i].y == player.y) {
                ok = 1;
        }
        if (maze_object[i].type == CHALICE &&
            maze_object[i].x == 255)
            has_chalice = 1;
        if (ok && has_chalice)
            break;
    }
    if (!ok)
        return 0;

    if (direction > 0 && maze_current_level >= NLEVELS - 1)
        return 0;
    else if (direction < 0 && maze_current_level <= 0) {
	if (!has_chalice) {
            encounter_text = "YOU MUST GET";
            encounter_adjective = "THE CHALICE";
            encounter_name = "FIRST";
            encounter_object = 255;
            return 0;
        }
    }

    maze_previous_level = maze_current_level;
    maze_current_level += direction;
    maze_program_state = MAZE_LEVEL_INIT;
    maze_player_initial_placement = placement;
    return 1;
}

static int go_up(void)
{
    return go_up_or_down(-1);
}

static int go_down(void)
{
    return go_up_or_down(1);
}

static int check_for_encounter(unsigned char newx, unsigned char newy)
{
    int i, monster;

    monster = 0;
    encounter_text = "x";
    for (i = 0; i < nmaze_objects; i++) {
        /* If we are just about to move onto a square where an object is... */
        if (maze_object[i].x == newx && maze_object[i].y == newy) {
            switch(maze_object_template[maze_object[i].type].category) {
            case MAZE_OBJECT_MONSTER:
                encounter_text = "YOU ENCOUNTER A";
                encounter_adjective = "";
                encounter_name = maze_object_template[maze_object[i].type].name;
                encounter_object = i;
                monster = 1;
                break;
            case MAZE_OBJECT_WEAPON:
                encounter_text = "YOU FOUND A";
                encounter_adjective = weapon_type[maze_object[i].tsd.weapon.type].adjective;
                encounter_name = weapon_type[maze_object[i].tsd.weapon.type].name;
                break;
            case MAZE_OBJECT_KEY:
            case MAZE_OBJECT_TREASURE:
            case MAZE_OBJECT_SCROLL:
            case MAZE_OBJECT_GRENADE:
                if (!monster) {
                    encounter_text = "YOU FOUND A";
                    encounter_adjective = "";
                    encounter_name = maze_object_template[maze_object[i].type].name;
                }
                break;
            case MAZE_OBJECT_ARMOR:
                if (!monster) {
                    encounter_text = "YOU FOUND A";
                    encounter_adjective = armor_type[maze_object[i].tsd.armor.type].adjective;
                    encounter_name = armor_type[maze_object[i].tsd.armor.type].name;
                }
                break;
            case MAZE_OBJECT_POTION:
                if (!monster) {
                    encounter_text = "YOU FOUND A";
                    encounter_adjective = potion_type[maze_object[i].tsd.potion.type].adjective;
                    encounter_name = "POTION";
                }
                break;
            case MAZE_OBJECT_DOWN_LADDER:
                if (!monster) {
                    encounter_text = "A LADDER";
                    encounter_adjective = "";
                    encounter_name = "LEADS DOWN";
                }
                break;
            case MAZE_OBJECT_UP_LADDER:
                if (!monster) {
                    encounter_text = "A LADDER";
                    encounter_adjective = "";
                    encounter_name = "LEADS UP";
                }
                break;
            case MAZE_OBJECT_CHALICE:
		add_achievement(ACHIEVEMENT_MAZE_CHALICE_FOUND, 1);
                encounter_text = "YOU FOUND THE";
                encounter_adjective = "CHALICE OF";
                encounter_name = "OBFUSCATION!";
                break;
            default:
                if (!monster) {
                    encounter_text = "YOU FOUND SOMETHING";
                    encounter_adjective = "";
                    encounter_name = maze_object_template[maze_object[i].type].name;
                }
                break;
            }
        }
    }
    return monster;
}

static void maze_menu_clear(void)
{
	dynmenu_clear(&maze_menu);
	dynmenu_set_colors(&maze_menu, GREEN, WHITE);
}

static void maze_button_pressed(void)
{
    int i;

    int newx, newy;
    int monster_present = 0;
    int takeable_object_count = 0;
    int droppable_object_count = 0;
    int grenade_count = 0;
    int grenade_number = -1;
    int scroll_number = -1;

    if (game_is_won) {
        maze_program_state = MAZE_GAME_INIT;
        return;
    }
    if (player.hitpoints == 0)
        return;
    if (maze_menu.menu_active) {
        maze_program_state = maze_menu.item[maze_menu.current_item].next_state;
        maze_menu.chosen_cookie = maze_menu.item[maze_menu.current_item].cookie;
        maze_menu.menu_active = 0;
        return;
    }
    newx = player.x + xoff[player.direction];
    newy = player.y + yoff[player.direction];
    maze_menu_clear();
    strcpy(maze_menu.title, "CHOOSE ACTION");
    for (i = 0; i < nmaze_objects; i++) {
        if (player.x == maze_object[i].x && player.y == maze_object[i].y) {
            switch(maze_object_template[maze_object[i].type].category) {
            case MAZE_OBJECT_DOWN_LADDER:
                 dynmenu_add_item(&maze_menu, "CLIMB DOWN", MAZE_STATE_GO_DOWN, 1);
                 break;
            case MAZE_OBJECT_UP_LADDER:
                 dynmenu_add_item(&maze_menu, "CLIMB UP", MAZE_STATE_GO_UP, 1);
                 break;
            case MAZE_OBJECT_MONSTER:
                 monster_present = 1;
                 break;
            case MAZE_OBJECT_WEAPON:
            case MAZE_OBJECT_KEY:
            case MAZE_OBJECT_POTION:
            case MAZE_OBJECT_TREASURE:
            case MAZE_OBJECT_ARMOR:
            case MAZE_OBJECT_SCROLL:
            case MAZE_OBJECT_GRENADE:
            case MAZE_OBJECT_CHALICE:
                 takeable_object_count++;
                 break;
            default:
                 break;
            }
        }
        if (maze_object[i].x == 255 && object_is_portable(i))
           droppable_object_count++;
	if (maze_object[i].x == 255 &&
           maze_object_template[maze_object[i].type].category == MAZE_OBJECT_GRENADE) {
           grenade_count++;
	   grenade_number = i;
	}
	if (maze_object[i].x == 255 &&
           maze_object_template[maze_object[i].type].category == MAZE_OBJECT_SCROLL) {
	   scroll_number = i;
	}
        if (maze_object[i].x == newx && maze_object[i].y == newy &&
            maze_object_template[maze_object[i].type].category == MAZE_OBJECT_MONSTER)
            monster_present = 1;
    }
    dynmenu_add_item(&maze_menu, "NEVER MIND", MAZE_RENDER, 1);
    if (monster_present) {
        dynmenu_add_item(&maze_menu, "FIGHT MONSTER!", MAZE_STATE_FIGHT, 1);
        dynmenu_add_item(&maze_menu, "FLEE!", MAZE_STATE_FLEE, 1);
    }
    if (takeable_object_count > 0)
        dynmenu_add_item(&maze_menu, "TAKE ITEM", MAZE_CHOOSE_TAKE_OBJECT, takeable_object_count);
    if (droppable_object_count > 0)
        dynmenu_add_item(&maze_menu, "DROP OBJECT",  MAZE_CHOOSE_DROP_OBJECT, 1);
    if (grenade_count > 0)
        dynmenu_add_item(&maze_menu, "THROW GRENADE",  MAZE_THROW_GRENADE, grenade_number);
    dynmenu_add_item(&maze_menu, "VIEW MAP", MAZE_DRAW_MAP, 1);
    dynmenu_add_item(&maze_menu, "WIELD WEAPON", MAZE_CHOOSE_WEAPON, 1);
    dynmenu_add_item(&maze_menu, "DON ARMOR", MAZE_CHOOSE_ARMOR, 1);
    dynmenu_add_item(&maze_menu, "READ SCROLL", MAZE_READ_SCROLL, scroll_number);
    dynmenu_add_item(&maze_menu, "QUAFF POTION", MAZE_CHOOSE_POTION, 1);
    dynmenu_add_item(&maze_menu, "EXIT GAME", MAZE_EXIT, 255);
    maze_menu.menu_active = 1;
    maze_program_state = MAZE_DRAW_MENU;
}

static void move_player_one_step(int direction)
{
    int newx, newy, dx, dy, dist, hp, str, damage;

    if (!combat_mode) {
       newx = player.x + xoff[direction];
       newy = player.y + yoff[direction];
       if (!out_of_bounds(newx, newy) && is_passage(newx, newy)) {
           if (!check_for_encounter(newx, newy)) {
               player.x = newx;
               player.y = newy;
           }
       }
   } else {

       newx = player.combatx + xoff[direction] * 4;
       newy = player.combaty + yoff[direction] * 4;
       if (newx < 10)
           newx = 10;
       if (newx > LCD_XSIZE - 10)
           newx = LCD_XSIZE - 10;
       if (newy < 10)
           newy = 10;
       if (newy > LCD_YSIZE - 10)
           newy = LCD_YSIZE - 10;
       player.combatx = newx;
       player.combaty = newy;
       dx = player.combatx - combatant.combatx;
       dy = player.combaty - combatant.combaty;
       dist = dx * dx + dy * dy;
       if (dist < 100) {
           str = xorshift(&xorshift_state) % 160;
           /* damage = xorshift(&xorshift_state) % maze_object_template[maze_object[player.weapon].type].damage; */
           if (player.weapon == 255) /* fists */
               damage = 1;
           else
               damage = xorshift(&xorshift_state) % weapon_type[maze_object[player.weapon].type].damage;
           combatant.combatx -= dx * (80 + str) / 100;
           combatant.combaty -= dy * (80 + str) / 100;
           if (combatant.combatx < 40)
              combatant.combatx += 40;
           if (combatant.combatx > LCD_XSIZE - 40)
              combatant.combatx -= 40;
           if (combatant.combaty < 40)
              combatant.combaty += 40;
           if (combatant.combaty > LCD_XSIZE - 40)
              combatant.combaty -= 40;
           hp = combatant.hitpoints - damage;
           if (hp < 0)
              hp = 0;
           combatant.hitpoints = hp;
           if (combatant.hitpoints == 0) {
               maze_program_state = MAZE_STATE_PLAYER_DEFEATS_MONSTER;
               if (strcmp(maze_object_template[maze_object[encounter_object].type].name, "DRAGON") == 0)
                   add_achievement(ACHIEVEMENT_MAZE_DRAGONS_SLAIN, 1);
               combat_mode = 0;
               maze_object[encounter_object].x = 255; /* Move it off the board */
           }
       }
   }
}

static void process_commands(void)
{
    int base_direction;

    base_direction = combat_mode ? 0 : player.direction;

    int down_latches = button_down_latches();
    if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
        maze_button_pressed();
    } else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
        if (maze_menu.menu_active)
            dynmenu_change_current_selection(&maze_menu, -1);
        else
            move_player_one_step(base_direction);
    } else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
        if (maze_menu.menu_active)
            dynmenu_change_current_selection(&maze_menu, 1);
        else
            move_player_one_step(normalize_direction(base_direction + 4));
    } else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
        if (combat_mode)
            move_player_one_step(6);
        else
            player.direction = left_dir(player.direction);
    } else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
        if (combat_mode)
            move_player_one_step(2);
        else
            player.direction = right_dir(player.direction);
    } else {
        return;
    }

    if (player.hitpoints == 0) {
        maze_program_state = MAZE_GAME_INIT;
    }

    if (maze_program_state == MAZE_PROCESS_COMMANDS) {
        if (maze_menu.menu_active)
            maze_program_state = MAZE_DRAW_MENU;
        else if (combat_mode)
            maze_program_state = MAZE_COMBAT_MONSTER_MOVE;
        else
            maze_program_state = MAZE_RENDER;
    }
}

static void draw_objects(void)
{
    int a, b, x[2], y[2], s, npoints;
    int i = current_drawing_object;
    int otype = maze_object[i].type;
    const struct point *drawing = maze_object_template[otype].drawing;
    int color;

    a = 0;
    b = 1;
    x[0] = player.x;
    y[0] = player.y;
    x[1] = player.x + xoff[player.direction] * 6;
    y[1] = player.y + yoff[player.direction] * 6;

    if (x[0] == x[1]) {
        if (y[0] > y[1]) {
            a = 1;
            b = 0;
        }
    } else if (y[0] == y[1]) {
        if (x[0] > x[1]) {
            a = 1;
            b = 0;
        }
    }

    npoints = maze_object_template[otype].npoints;
    color = maze_object_template[otype].color;
    if (x[0] == x[1]) {
        if (maze_object[i].x == x[0] && maze_object[i].y >= y[a] && maze_object[i].y <= y[b]) {
            s = abs(maze_object[i].y - player.y);
            if (s >= maze_object_distance_limit)
                goto next_object;
            draw_object(drawing, npoints, s, color, LCD_XSIZE / 2, LCD_YSIZE / 2);
        }
    } else if (y[0] == y[1]) {
        if (maze_object[i].y == y[0] && maze_object[i].x >= x[a] && maze_object[i].x <= x[b]) {
            s = abs(maze_object[i].x - player.x);
            if (s >= maze_object_distance_limit)
                goto next_object;
            draw_object(drawing, npoints, s, color, LCD_XSIZE / 2, LCD_YSIZE / 2);
        }
    }

next_object:
    current_drawing_object++;
    if (current_drawing_object >= nmaze_objects) {
        current_drawing_object = 0;
        maze_program_state = MAZE_RENDER_ENCOUNTER;
    }
}

static int maze_render_step = 0;

static void render_maze(int color)
{
    int x, y, ox, oy, left, right;
    const int steps = 7;
    int hit_back_wall = 0;

    if (maze_render_step == 0) {
        FbClear();
        maze_object_distance_limit = steps;
    }

    FbColor(color);

    ox = player.x + xoff[player.direction] * maze_render_step;
    oy = player.y + yoff[player.direction] * maze_render_step;

    /* Draw the wall we might be facing */
    x = ox;
    y = oy;
    if (!out_of_bounds(x, y)) {
        if (!is_passage(x, y)) {
            draw_forward_wall(maze_start, (maze_scale * 80) / 100);
            hit_back_wall = 1;
        } else {
            mark_maze_square_visited(ox, oy);
        }
    }
    /* Draw the wall or passage to our left */
    left = left_dir(player.direction);
    x = ox + xoff[left];
    y = oy + yoff[left];
    if (!out_of_bounds(x, y) && !hit_back_wall) {
        if (is_passage(x, y)) {
            draw_left_passage(maze_start, maze_scale);
            mark_maze_square_visited(x, y);
        } else {
            draw_left_wall(maze_start, maze_scale);
        }
    }
    /* Draw the wall or passage to our right */
    right = right_dir(player.direction);
    x = ox + xoff[right];
    y = oy + yoff[right];
    if (!out_of_bounds(x, y) && !hit_back_wall) {
        if (is_passage(x, y)) {
            draw_right_passage(maze_start, maze_scale);
            mark_maze_square_visited(x, y);
        } else {
            draw_right_wall(maze_start, maze_scale);
        }
    }
    /* Advance forward ahead of the player in our rendering */
    maze_start = maze_start + maze_scale;
    maze_scale = (maze_scale * 819) >> 10; /* Approximately multiply by 0.8 */
    if (hit_back_wall) { /* If we are facing a wall, do not draw beyond that wall. */
        maze_back_wall_distance = maze_render_step; /* used by draw_objects */
        maze_object_distance_limit = maze_render_step;
        maze_render_step = steps;
    }
    maze_render_step++;
    if (maze_render_step >= steps) {
        maze_render_step = 0;
        maze_back_wall_distance = steps;
        maze_program_state = MAZE_OBJECT_RENDER;
        maze_start = 0;
        maze_scale = 12;
   }
}

static void draw_encounter(void)
{
    maze_program_state = MAZE_DRAW_STATS;

    if (encounter_text[0] == 'x')
        return;
    FbColor(WHITE);
    FbMove(10, 90);
    FbWriteLine(encounter_text);
    FbMove(10, 100);
    if (strcmp(encounter_adjective, "") != 0) {
        FbWriteLine(encounter_adjective);
        FbMove(10, 110);
    }
    FbWriteLine(encounter_name);
}

static void maze_combat_monster_move(void)
{
    int dx, dy, dist, speed, str, damage, hp, protection;

    dx = player.combatx - combatant.combatx;
    dy = player.combaty - combatant.combaty;
    dist = dx * dx + dy * dy;
    speed = maze_object[encounter_object].tsd.monster.speed;

    if (dist > 100) {
        if (dx < 0)
            combatant.combatx -= speed;
        else if (dx > 0)
            combatant.combatx += speed;
        if (dy < 0)
            combatant.combaty -= speed;
        else if (dy > 0)
            combatant.combaty += speed;
    } else {
        str = xorshift(&xorshift_state) % 160;

        if (player.armor == 255)
            protection = 0;
        else
            protection = xorshift(&xorshift_state) % armor_type[maze_object[player.armor].tsd.armor.type].protection;

        damage = xorshift(&xorshift_state) % maze_object_template[maze_object[encounter_object].type].damage;
        damage = damage - protection;
        if (damage < 0)
            damage = 0;

        player.combatx += dx * (80 + str) / 100;
        player.combaty += dy * (80 + str) / 100;
        if (player.combatx < 40)
           player.combatx += 40;
        if (player.combatx > LCD_XSIZE - 40)
           player.combatx -= 40;
        if (player.combaty < 40)
           player.combaty += 40;
        if (player.combaty > LCD_XSIZE - 40)
           player.combaty -= 40;
        hp = player.hitpoints - damage;
        if (hp < 0)
           hp = 0;
        player.hitpoints = hp;
        if (player.hitpoints == 0) {
            maze_program_state = MAZE_STATE_PLAYER_DIED;
            return;
        }
    }
    maze_program_state = MAZE_RENDER_COMBAT;
}

static void maze_render_combat(void)
{
    int color, npoints;
    int otype = maze_object[encounter_object].type;
    const struct point *drawing = maze_object_template[otype].drawing;

    FbClear();

    /* Draw the monster */
    color = maze_object_template[otype].color;
    npoints = maze_object_template[otype].npoints;
    draw_object(drawing, npoints, ARRAYSIZE(drawing_scale_numerator) - 1,
                color, combatant.combatx, combatant.combaty);

    /* Draw the player */
    draw_object(player_points, ARRAYSIZE(player_points), ARRAYSIZE(drawing_scale_numerator) - 1,
                WHITE, player.combatx, player.combaty);

    maze_program_state = MAZE_DRAW_STATS;
}

static void maze_player_defeats_monster(void)
{
    FbClear();
    FbColor(RED);
    FbMove(10, LCD_YSIZE / 2 - 10);
    FbWriteLine("YOU KILL THE");
    FbMove(10, LCD_YSIZE / 2);
    FbWriteLine(encounter_name);
    FbMove(10, LCD_YSIZE / 2 + 10);
    if (player.weapon == 255) {
        FbWriteLine("WITH YOUR FISTS");
    } else {
        FbWriteLine("WITH THE");
        FbMove(10, LCD_YSIZE / 2 + 20);
        FbWriteLine(weapon_type[maze_object[player.weapon].tsd.weapon.type].adjective);
        FbMove(10, LCD_YSIZE / 2 + 30);
        FbWriteLine(weapon_type[maze_object[player.weapon].tsd.weapon.type].name);
    }
    encounter_text = "x";
    encounter_adjective = "";
    encounter_name = "x";
    combat_mode = 0;
    maze_program_state = MAZE_SCREEN_RENDER;
}

static void maze_player_died(void)
{
    FbClear();
    FbColor(RED);
    FbMove(10, LCD_YSIZE - 20);
    FbWriteLine("YOU HAVE DIED");
    draw_object(bones_points, ARRAYSIZE(bones_points), 0, WHITE, LCD_XSIZE / 2, LCD_YSIZE / 2);
    encounter_text = "x";
    encounter_adjective = "";
    encounter_name = "x";
    combat_mode = 0;
    maze_current_level = 0;
    maze_previous_level = -1;
    maze_player_initial_placement = MAZE_PLACE_PLAYER_BENEATH_UP_LADDER;
    maze_program_state = MAZE_SCREEN_RENDER;
}

static void maze_choose_potion(void)
{
    int i;
    char name[20];

    maze_menu_clear();
    maze_menu.menu_active = 1;
    strcpy(maze_menu.title, "CHOOSE POTION");

    for (i = 0; i < nmaze_objects; i++) {
        if (maze_object_template[maze_object[i].type].category != MAZE_OBJECT_POTION)
            continue;
        if (maze_object[i].x == 255 ||
            (maze_object[i].x == player.x && maze_object[i].y == player.y)) {
            strcpy(name, potion_type[maze_object[i].tsd.potion.type].adjective);
            strcat(name, " POTION");
            dynmenu_add_item(&maze_menu, name, MAZE_QUAFF_POTION, i);
        }
    }
    dynmenu_add_item(&maze_menu, "NEVER MIND", MAZE_RENDER, 255);
    maze_program_state = MAZE_DRAW_MENU;
}

static void maze_quaff_potion(void)
{
    short hp, delta, object, ptype;
    char *feel;

    if (maze_menu.chosen_cookie == 255) { /* never mind */
        maze_program_state = MAZE_RENDER;
    }
    object = maze_menu.chosen_cookie;
    ptype = maze_object[object].tsd.potion.type;
    delta = potion_type[ptype].health_impact;
    maze_object[object].x = 254; /* off maze, but not in pocket, "use up" the potion */
    hp = player.hitpoints + delta;
    if (hp > 255)
        hp = 255;
    if (hp < 0)
        hp = 0;
    delta = hp - player.hitpoints;
    if (delta < 0)
       feel = "WORSE";
    else if (delta > 0)
       feel = "BETTER";
    else
       feel = "NOTHING";
    player.hitpoints = hp;
    FbClear();
    FbMove(10, LCD_YSIZE / 2);
    FbWriteLine("YOU FEEL");
    FbMove(10, LCD_YSIZE / 2 + 10);
    FbWriteLine(feel);
    maze_program_state = MAZE_SCREEN_RENDER;
}

static void maze_choose_weapon(void)
{
    int i;
    char name[20];

    maze_menu_clear();
    maze_menu.menu_active = 1;
    strcpy(maze_menu.title, "WIELD WEAPON");

    for (i = 0; i < nmaze_objects; i++) {
        if (maze_object_template[maze_object[i].type].category != MAZE_OBJECT_WEAPON)
            continue;
        if (maze_object[i].x == 255 ||
            (maze_object[i].x == player.x && maze_object[i].y == player.y)) {
            if (player.weapon == i)
               strcpy(name, "+");
            else
               strcpy(name, " ");
            strcat(name, weapon_type[maze_object[i].tsd.weapon.type].adjective);
            strcat(name, " ");
            strcat(name, weapon_type[maze_object[i].tsd.weapon.type].name);
            dynmenu_add_item(&maze_menu, name, MAZE_WIELD_WEAPON, i);
        }
    }
    dynmenu_add_item(&maze_menu, "NEVER MIND", MAZE_RENDER, 255);
    maze_program_state = MAZE_DRAW_MENU;
}

static void maze_wield_weapon(void)
{
    if (maze_menu.chosen_cookie == 255) { /* never mind */
        maze_program_state = MAZE_RENDER;
    }
    player.weapon = maze_menu.chosen_cookie;
    maze_object[player.weapon].x = 255; /* In case we wield directly from dungeon floor */
    FbClear();
    FbMove(10, LCD_YSIZE / 2);
    FbWriteLine("YOU WIELD THE");
    FbMove(10, LCD_YSIZE / 2 + 10);
    FbWriteLine(weapon_type[maze_object[player.weapon].tsd.weapon.type].adjective);
    FbMove(10, LCD_YSIZE / 2 + 20);
    FbWriteLine(weapon_type[maze_object[player.weapon].tsd.weapon.type].name);
    maze_program_state = MAZE_SCREEN_RENDER;
}

static void maze_choose_armor(void)
{
    int i;
    char name[20];

    maze_menu_clear();
    maze_menu.menu_active = 1;
    strcpy(maze_menu.title, "DON ARMOR");

    for (i = 0; i < nmaze_objects; i++) {
        if (maze_object_template[maze_object[i].type].category != MAZE_OBJECT_ARMOR)
            continue;
        if (maze_object[i].x == 255 ||
            (maze_object[i].x == player.x && maze_object[i].y == player.y)) {
            if (player.armor == i)
               strcpy(name, "+");
            else
               strcpy(name, " ");
            strcat(name, armor_type[maze_object[i].tsd.armor.type].adjective);
            strcat(name, " ");
            strcat(name, armor_type[maze_object[i].tsd.armor.type].name);
            dynmenu_add_item(&maze_menu, name, MAZE_DON_ARMOR, i);
        }
    }
    dynmenu_add_item(&maze_menu, "NEVER MIND", MAZE_RENDER, 255);
    maze_program_state = MAZE_DRAW_MENU;
}

static void maze_don_armor(void)
{
    if (maze_menu.chosen_cookie == 255) { /* never mind */
        maze_program_state = MAZE_RENDER;
    }
    player.armor = maze_menu.chosen_cookie;
    maze_object[player.armor].x = 255; /* In case we don directly from dungeon floor */
    FbClear();
    FbMove(10, LCD_YSIZE / 2);
    FbWriteLine("YOU DON THE");
    FbMove(10, LCD_YSIZE / 2 + 10);
    FbWriteLine(armor_type[maze_object[player.armor].tsd.armor.type].adjective);
    FbMove(10, LCD_YSIZE / 2 + 20);
    FbWriteLine(armor_type[maze_object[player.armor].tsd.armor.type].name);
    maze_program_state = MAZE_SCREEN_RENDER;
}

static void maze_draw_stats(void)
{
    char gold_pieces[10], hitpoints[10];

    FbColor(WHITE);
    sprintf( gold_pieces, "%d", player.gp);
    sprintf( hitpoints, "%d", player.hitpoints);
    FbMove(2, 122);
    if (player.hitpoints < 50)
       FbColor(RED);
    else if (player.hitpoints < 100)
       FbColor(YELLOW);
    else
       FbColor(GREEN);
    FbWriteLine("HP:");
    FbMove(2 + 24, 122);
    FbWriteLine(hitpoints);
    FbColor(YELLOW);
    FbMove(55, 122);
    FbWriteLine("GP:");
    FbMove(55 + 24, 122);
    FbWriteLine(gold_pieces);

    maze_program_state = MAZE_SCREEN_RENDER;
}

static void init_seeds(void)
{
    int i;

    for (i = 0; i < NLEVELS; i++)
        maze_random_seed[i] = xorshift(&xorshift_state);
}

static void potions_init(void)
{
   int i;

   for (i = 0; (size_t) i < ARRAYSIZE(potion_type); i++)
       potion_type[i].health_impact = (xorshift(&xorshift_state) % 50) - 15;
}

static void maze_game_init(void)
{
#ifdef __linux__
    struct timeval tv;

    gettimeofday(&tv, NULL);
    xorshift_state = tv.tv_usec;
#endif
    if (xorshift_state == 0)
        xorshift_state = 0xa5a5a5a5;

    dynmenu_init(&maze_menu, maze_menu_item, ARRAYSIZE(maze_menu_item));
    maybe_load_achievements_from_flash();
    init_seeds();
    player_init();
    potions_init();

    game_is_won = 0;

    maze_program_state = MAZE_GAME_START_MENU;
}

static void maze_draw_menu(void)
{
    dynmenu_draw(&maze_menu);
    maze_program_state = MAZE_SCREEN_RENDER;
}

static void maze_game_start_menu(void)
{

    maze_menu_clear();

    maze_menu.menu_active = 1;
    strcpy(maze_menu.title, "SEEK YE THE");
    strcpy(maze_menu.title2, "CHALICE OF");
    strcpy(maze_menu.title3, "OBFUSCATION!");
    dynmenu_add_item(&maze_menu, "NEW GAME", MAZE_LEVEL_INIT, 0);
    dynmenu_add_item(&maze_menu, "EXIT GAME", MAZE_EXIT, 0);

    maze_program_state = MAZE_DRAW_MENU;
}

static void maze_begin_fight()
{
    combat_mode = 1;
    player.combatx = LCD_XSIZE / 2;
    player.combaty = LCD_YSIZE - 40;
    combatant.combatx = LCD_XSIZE / 2;
    combatant.combaty = 50;
    combatant.hitpoints = maze_object[encounter_object].tsd.monster.hitpoints;
    maze_program_state = MAZE_RENDER_COMBAT;
}

static void maze_choose_take_or_drop_object(char *title, enum maze_program_state_t next_state)
{
    int i, limit;
    char name[20];

    maze_menu_clear();
    maze_menu.menu_active = 1;
    strcpy(maze_menu.title, title);

    limit = nmaze_objects;
    if (limit > 10)
        limit = 10;
    for (i = 0; i < nmaze_objects; i++) {
        if ((next_state == MAZE_TAKE_OBJECT && maze_object[i].x == player.x && maze_object[i].y == player.y) ||
            (next_state == MAZE_DROP_OBJECT && maze_object[i].x == 255)) {
            switch(maze_object_template[maze_object[i].type].category) {
            case MAZE_OBJECT_WEAPON:
                if (i == player.weapon)
                    strcpy(name, "+");
                else
                    strcpy(name, " ");
                strcat(name, weapon_type[maze_object[i].tsd.weapon.type].adjective);
                strcat(name, " ");
                strcat(name, weapon_type[maze_object[i].tsd.weapon.type].name);
                break;
            case MAZE_OBJECT_KEY:
                strcpy(name, "KEY");
                break;
            case MAZE_OBJECT_POTION:
                strcpy(name, potion_type[maze_object[i].tsd.potion.type].adjective);
                strcat(name, " POTION");
                break;
            case MAZE_OBJECT_TREASURE:
                strcpy(name, "CHEST");
                break;
            case MAZE_OBJECT_ARMOR:
                if (i == player.armor)
                    strcpy(name, "+");
                else
                    strcpy(name, " ");
                strcat(name, armor_type[maze_object[i].tsd.armor.type].adjective);
                strcat(name, " ");
                strcat(name, armor_type[maze_object[i].tsd.armor.type].name);
                break;
            case MAZE_OBJECT_SCROLL:
                strcpy(name, "SCROLL");
                break;
            case MAZE_OBJECT_GRENADE:
                strcpy(name, "GRENADE");
                break;
            case MAZE_OBJECT_CHALICE:
                strcpy(name, "CHALICE");
                break;
            default:
                continue;
            }
            dynmenu_add_item(&maze_menu, name, next_state, i);
            limit--;
            if (limit == 0) /* Don't make the menu too big. */
                break;
        }
    }
    dynmenu_add_item(&maze_menu, "NEVER MIND", MAZE_RENDER, 255);
    maze_program_state = MAZE_DRAW_MENU;
}

static void maze_choose_take_object(void)
{
    maze_choose_take_or_drop_object("TAKE OBJECT", MAZE_TAKE_OBJECT);
}

static void maze_choose_drop_object(void)
{
    maze_choose_take_or_drop_object("DROP OBJECT", MAZE_DROP_OBJECT);
}

static void maze_take_object(void)
{
    int i;

    i = maze_menu.chosen_cookie;
    maze_object[i].x = 255; /* Take object */
    maze_program_state = MAZE_RENDER;

    switch(maze_object_template[maze_object[i].type].category) {
    case MAZE_OBJECT_TREASURE:
        player.gp += maze_object[i].tsd.treasure.gp;
        maze_object[i].tsd.treasure.gp = 0; /* you can't get infinite gp by repeatedly dropping and taking treasure */
        break;
    case MAZE_OBJECT_WEAPON:
        if (player.weapon == 255) /* bare handed? Auto wield weapon. */
            maze_program_state = MAZE_WIELD_WEAPON;
        break;
    case MAZE_OBJECT_ARMOR:
        if (player.armor == 255) /* bare chested? Auto don armor. */
            maze_program_state = MAZE_DON_ARMOR;
        break;
    default:
        break;
    }
}

static void maze_win_condition(void)
{
    FbClear();
    FbColor(WHITE);
    FbMove(10, 30);
    FbWriteLine("YOU HAVE");
    FbMove(10, 40);
    FbWriteLine("ESCAPED ALIVE");
    FbMove(10, 50);
    FbWriteLine("WITH THE");
    FbMove(10, 60);
    FbWriteLine("CHALICE OF");
    FbMove(10, 70);
    FbWriteLine("OBFUSCATION!");
    FbMove(10, 90);
    FbWriteLine("YOU WIN!");
    maze_menu.menu_active = 0;
    encounter_text = "x";
    encounter_adjective = "";
    encounter_name = "x";
    combat_mode = 0;
    maze_current_level = 0;
    maze_previous_level = -1;
    maze_player_initial_placement = MAZE_PLACE_PLAYER_BENEATH_UP_LADDER;
    maze_program_state = MAZE_SCREEN_RENDER;
    add_achievement(ACHIEVEMENT_MAZE_CHALICE_RECOVERED, 1);
}

static void maze_drop_object(void)
{
    int i;

    i = maze_menu.chosen_cookie;
    maze_object[i].x = player.x;
    maze_object[i].y = player.y;
    maze_program_state = MAZE_RENDER;
    if (player.weapon == i)
        player.weapon = 255;
    if (player.armor == i)
       player.armor = 255;
}

static void maze_throw_grenade(void)
{
    int i;

    i = maze_menu.chosen_cookie;
    if (i < 0 || (size_t) i >= ARRAYSIZE(maze_object)) {
	maze_program_state = MAZE_RENDER;
	return;
    }
    maze_object[i].x = 254; /* Remove grenade from player's possession */
    maze_object[i].y = 254;
    encounter_text = "THE GRENADE";
    encounter_adjective = "EMITS THOUGHTS";
    encounter_name = "AND PRAYERS!";
    maze_program_state = MAZE_RENDER;
}

static void maze_read_scroll(void)
{
    int i;

    i = maze_menu.chosen_cookie;
    if (i < 0 || (size_t) i >= ARRAYSIZE(maze_object)) {
	maze_program_state = MAZE_RENDER;
	return;
    }
    maze_object[i].x = 254; /* Remove scroll from player's possession */
    maze_object[i].y = 254;
    encounter_text = scroll[i % NSCROLLS].text1;
    encounter_adjective = scroll[i % NSCROLLS].text2;
    encounter_name = scroll[i % NSCROLLS].text3;
    maze_program_state = MAZE_RENDER;
}

int maze_cb(void)
{
    switch (maze_program_state) {
    case MAZE_GAME_INIT:
        maze_game_init();
        break;
    case MAZE_GAME_START_MENU:
        maze_game_start_menu();
        break;
    case MAZE_LEVEL_INIT:
        maze_init();
        break;
    case MAZE_BUILD:
        generate_maze();
        break;
    case MAZE_PRINT:
        print_maze();
        break;
    case MAZE_RENDER:
        render_maze(level_color[maze_current_level % 3]);
        break;
    case MAZE_OBJECT_RENDER:
        draw_objects();
        break;
    case MAZE_RENDER_ENCOUNTER:
        draw_encounter();
        break;
    case MAZE_SCREEN_RENDER:
        FbSwapBuffers();
        maze_program_state = MAZE_PROCESS_COMMANDS;
        break;
    case MAZE_PROCESS_COMMANDS:
        process_commands();
        break;
    case MAZE_DRAW_MAP:
        draw_map();
        break;
    case MAZE_DRAW_STATS:
        maze_draw_stats();
        break;
    case MAZE_DRAW_MENU:
        maze_draw_menu();
        break;
    case MAZE_STATE_GO_DOWN:
         if (!go_down())
            maze_program_state = MAZE_RENDER;
         break;
    case MAZE_STATE_GO_UP:
        if (!go_up())
            maze_program_state = MAZE_RENDER;
        if (maze_current_level == -1) /* They have escaped the maze with the chalice */
            maze_program_state = MAZE_WIN_CONDITION;
        break;
    case MAZE_STATE_FIGHT:
         maze_begin_fight();
         break;
    case MAZE_STATE_FLEE:
         maze_program_state = MAZE_RENDER;
         break;
    case MAZE_COMBAT_MONSTER_MOVE:
         maze_combat_monster_move();
         break;
    case MAZE_RENDER_COMBAT:
         maze_render_combat();
         break;
    case MAZE_STATE_PLAYER_DEFEATS_MONSTER:
         maze_player_defeats_monster();
         break;
    case MAZE_STATE_PLAYER_DIED:
         maze_player_died();
         break;
    case MAZE_CHOOSE_POTION:
         maze_choose_potion();
         break;
    case MAZE_QUAFF_POTION:
         maze_quaff_potion();
         break;
    case MAZE_CHOOSE_WEAPON:
         maze_choose_weapon();
         break;
    case MAZE_WIELD_WEAPON:
         maze_wield_weapon();
         break;
    case MAZE_CHOOSE_ARMOR:
         maze_choose_armor();
         break;
    case MAZE_DON_ARMOR:
         maze_don_armor();
         break;
    case MAZE_CHOOSE_TAKE_OBJECT:
         maze_choose_take_object();
         break;
    case MAZE_CHOOSE_DROP_OBJECT:
         maze_choose_drop_object();
         break;
    case MAZE_TAKE_OBJECT:
         maze_take_object();
         break;
    case MAZE_DROP_OBJECT:
         maze_drop_object();
         break;
    case MAZE_THROW_GRENADE:
         maze_throw_grenade();
         break;
    case MAZE_READ_SCROLL:
         maze_read_scroll();
         break;
    case MAZE_EXIT:
        maze_program_state = MAZE_GAME_INIT;
        returnToMenus();
        break;
    case MAZE_WIN_CONDITION:
        game_is_won = 1;
        maze_win_condition();
        break;
    }
    return 0;
}


