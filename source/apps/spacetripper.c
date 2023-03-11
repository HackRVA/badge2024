/*********************************************

 Basic star trek type game

 Author: Stephen M. Cameron <stephenmcameron@gmail.com>
 (c) 2021 Stephen M. Cameron

**********************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "rtc.h"
#include "dynmenu.h"
#include "xorshift.h"
#include "trig.h"

static unsigned int xorshift_state = 0xa5a5a5a5;

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#define NKLINGONS 10
#define NCOMMANDERS 5
#define NROMULANS 5 
#define NPLANETS 20
#define NBLACKHOLES 10
#define NSTARBASES 10
#define NSTARS 20
#define NTOTAL (NKLINGONS + NCOMMANDERS + NROMULANS + NPLANETS + NBLACKHOLES + NSTARBASES + NSTARS)

#define INITIAL_TORPEDOES 10
#define INITIAL_ENERGY 5000
#define MAX_SHIELD_ENERGY 2500
#define INITIAL_DILITHIUM 100
#define WARP_ENERGY_PER_SEC 4
#define PHASER_ENERGY 3 /* multiplied by energy setting */
#define TORPEDO_ENERGY 10
#define LRS_ENERGY 2
#define STEERING_ENERGY_PER_DEG 5
#define SHIELDS_UP_ENERGY 10
#define TRANSPORTER_ENERGY 100

#define ENEMY_SHIP 'E'
#define PLANET 'P'
#define BLACKHOLE 'B'
#define STARBASE 'S'
#define STAR '*'

static char *torpedo_name = "TORPEDO";
static char *phaser_name = "PHASER";

static int get_time(void)
{
    return (int) rtc_get_time_of_day().tv_sec;
}

static const char *object_type_name(char object_type)
{
	switch (object_type) {
	case ENEMY_SHIP:
		return "AN ENEMY\nSHIP";
		break;
	case PLANET:
		return "A PLANET";
		break;
	case BLACKHOLE:
		return "A BLACK HOLE";
		break;
	case STAR:
		return "A STAR";
		break;
	case STARBASE:
		return "A STARBASE";
		break;
	default:
		return "AN UNKNOWN\nOBJECT";
		break;
	}
}

/* Program states.  Initial state is MAZE_GAME_INIT */
enum st_program_state_t {
	ST_GAME_INIT = 0,
	ST_NEW_GAME,
	ST_CAPTAIN_MENU,
	ST_PROCESS_INPUT,
	ST_LRS,
	ST_STAR_CHART,
	ST_SRS,
	ST_SRS_LEGEND,
	ST_SET_COURSE,
	ST_AIM_WEAPONS,
	ST_CHOOSE_ANGLE_INPUT,
	ST_DRAW_ANGLE_CHOOSER,
	ST_CHOOSE_WEAPONS,
	ST_PHOTON_TORPEDOES,
	ST_PHASER_BEAMS,
	ST_PHASER_POWER,
	ST_PHASER_POWER_INPUT,
	ST_FIRE_PHASER,
	ST_WARP,
	ST_WARP_INPUT,
	ST_SET_WEAPONS_BEARING,
	ST_DRAW_MENU,
	ST_RENDER_SCREEN,
	ST_EXIT,
	ST_PLANETS,
	ST_SENSORS,
	ST_DAMAGE_REPORT,
	ST_STATUS_REPORT,
	ST_ALERT,
	ST_DOCK,
	ST_STANDARD_ORBIT,
	ST_TRANSPORTER,
	ST_MINE_DILITHIUM,
	ST_LOAD_DILITHIUM,
	ST_SELF_DESTRUCT_CONFIRM,
	ST_SELF_DESTRUCT,
	ST_SHIELD_CONTROL,
	ST_SHIELDS_UP,
	ST_SHIELDS_DOWN,
	ST_SHIELD_ENERGY,
	ST_SHIELD_ENERGY_INPUT,
	ST_SHIELD_EXEC_ENERGY_XFER,
	ST_HULL_DESTROYED,
	ST_LIFE_SUPPORT_FAILED,
	ST_PLAYER_WON,
	ST_MOVE_WEAPON,
};

static struct dynmenu menu;
static struct dynmenu_item menu_item[20];

static enum st_program_state_t st_program_state = ST_GAME_INIT;

struct enemy_ship {
	unsigned char hitpoints;
	char shiptype;
	char flags;
#define NEAR_PLAYER 0x01
};

const char *planet_class = "MNOGRVI?";
static char *planet_class_name[] = {
		"HUMAN SUITABLE",
		"LOW GRAVITY",
		"MOSTLY WATER",
		"GAS GIANT",
		"ROCKY PLANETOID",
		"VENUS-LIKE",
		"ICEBALL/COMET",
};
struct planet {
	char flags;
#define PLANET_INHABITED	(1 << 0)
#define PLANET_KNOWN		(1 << 1)
#define PLANET_SCANNED		(1 << 2)
#define PLANET_HAS_DILITHIUM	(1 << 3)
#define PLANET_UNUSED_FLAG	(1 << 4);
#define PLANET_CLASS(x) (x >> 5) & 0x7
};

const char *star_class = "OBAFGKM";
struct star {
	/* Low three bits index into star_class for Morgan-keenan system. */
	/* bits 3 - 6 are 0 - 9 indicating temperature. */
	/* See https://en.wikipedia.org/wiki/Stellar_classification */
	char class;
};

union type_specific_data {
	struct enemy_ship ship;
	struct planet planet;
	struct star star;
};

struct game_object {
	int x, y; /* 32 bits, 16 for sector, 16 for quadrant, see coord_to_sector(), coord_to_quadrant() */
	union type_specific_data tsd;
	char type;
};

/* "HULL" must be last in ship_system[] to exclude it from repair by ship's crew. */
static const char *ship_system[] = { "WARP", "SHIELDS", "LIFE SUPP", "PHASERS", "TORP TUBE", "HULL"};
#define NSHIP_SYSTEMS ARRAYSIZE(ship_system)
#define WARP_SYSTEM 0
#define SHIELD_SYSTEM 1
#define LIFE_SUPP_SYSTEM 2
#define PHASER_SYSTEM 3
#define TORPEDO_SYSTEM 4
#define HULL_SYSTEM 5

struct player_ship {
	int x, y; /* 32 bits, 16 for sector, 16 for quadrant */
	int heading, new_heading; /* in degrees */
	int weapons_aim, new_weapons_aim; /* in degrees */
	int energy;
	unsigned char torpedoes;
	int warp_factor, new_warp_factor; /* 0 to (1 << 16) */
#define TORPEDO_POWER (1 << 18) /* energy delivered to target, not required to fire */
	int phaser_power, new_phaser_power; /* 0 to (1 << 16) */
	int shield_xfer, new_shield_xfer;
#define WARP10 (1 << 16)
#define WARP1 (WARP10 / 10)
	unsigned char shields;
	unsigned char shields_up;
	unsigned char dilithium_crystals;
	unsigned char damage[NSHIP_SYSTEMS];
	unsigned char docked;
	unsigned char standard_orbit;
	unsigned char away_team;
	unsigned char away_teams_crystals;
	unsigned char mined_dilithium;
	unsigned char damage_flags;
	unsigned char life_support_reserves;
	unsigned char alive;
};

static inline int warp_factor(int wf)
{
	return (wf / WARP1);
}

static inline int warp_factor_frac(int wf)
{
	return 10 * (wf % WARP1) / WARP1;
}

static inline int impulse_factor(int wf)
{
	return 10 * wf / WARP1;
}

/* Struct to keep state for unrolled loop for weapon motion / collision detection */
struct weapon_t {
	int i; /* iterator */
#define WEAPON_LIFETIME 20
	int x, y; /* position */
#define WEAPON_SPEED (3 * WARP1)
	int dx, dy; /* velocity */
	int damage_factor;
	int weapon_power;
	char *name;
};

struct angle_chooser_t {
	char *cur_angle_name, *new_angle_name, *set_angle_name;
	int *angle, *new_angle; /* in degrees */
	int old_angle, old_new_angle;
	int finished;
	void (*show_extra_data)(void);
};

struct game_state {
	struct game_object object[NTOTAL];
	struct player_ship player;
#define TICKS_PER_DAY 20 /* 30 days, 1 tick per second, 20 ticks per day == 10 minutes. */
#define START_DATE (2623)
	short stardate;
	short enddate;
	unsigned char srs_needs_update;
	unsigned char last_screen;
#define LRS_SCREEN 1
#define SRS_SCREEN 2
#define WARP_SCREEN 3
#define HEADING_SCREEN 4
#define SENSORS_SCREEN 5
#define DAMAGE_SCREEN 6
#define STATUS_SCREEN 7
#define PLANETS_SCREEN 8
#define CAPN_SCREEN 9
#define ALERT_SCREEN 10
#define AIMING_SCREEN 11
#define PHASER_POWER_SCREEN 12
#define STAR_CHART_SCREEN 13
#define REPORT_DAMAGE_SCREEN 14
#define SRS_LEGEND_SCREEN 15
#define UNKNOWN_SCREEN 255;

	struct weapon_t weapon;
	int score;
#define KLINGON_POINTS 100
#define ROMULAN_POINTS 200
#define COMMANDER_POINTS 400
	unsigned char game_over;
	unsigned char last_capn_menu_item;
	unsigned char visited_sectors[8];
	struct angle_chooser_t angle_chooser;
} gs = { 0 };

static inline int coord_to_sector(int c)
{
	return (c >> 16);
}

static inline int coord_to_quadrant(int c)
{
	return (c >> 13) & 0x07;
}

static void mark_sector_visited(unsigned char sx, unsigned char sy)
{
	gs.visited_sectors[sy] |= (1 << sx);
}

static int sector_visited(unsigned char sx, unsigned char sy)
{
	return gs.visited_sectors[sy] & (1 << sx);
}

static int find_free_obj(void)
{
	int i;

	for (i = 0; i < NTOTAL; i++) {
		if (gs.object[i].type == 0)
			return i;
	}
	return -1;
}

static int add_object(unsigned char type, int x, int y)
{
	int i;

	i = find_free_obj();
	if (i < 0)
		return -1;
	gs.object[i].type = type;
	gs.object[i].x = x;
	gs.object[i].y = y;
	return i;
}

static void delete_object(int i)
{
	if (i < 0 || i >= NTOTAL)
		return;
	memset(&gs.object[i], 0, sizeof(gs.object[i]));
}

static void reduce_player_energy(int amount)
{
	if (gs.player.energy == 0)
		return;

	if (gs.player.dilithium_crystals < 10)
		amount *= 2;
	else if (gs.player.dilithium_crystals < 5)
		amount *= 4;
	else if (gs.player.dilithium_crystals == 0)
		amount *= 10;
	gs.player.energy -= amount;
	if (gs.player.energy <= 0) {
		gs.player.energy = 0;
		gs.player.warp_factor = 0;
	}
}

static void alert_player(char *title, char *msg)
{
	FbClear();
	FbMove(2, 0);
	FbColor(WHITE);
	FbWriteLine(title);
	FbColor(GREEN);
	FbMove(2, 20);
	FbWriteString(msg);
	FbSwapBuffers();
	gs.last_screen = ALERT_SCREEN;
	st_program_state = ST_ALERT;
}

static void clear_menu(void)
{
	dynmenu_clear(&menu);
	dynmenu_set_colors(&menu, GREEN, WHITE);
}

static void setup_main_menu(void)
{
	clear_menu();
	strcpy(menu.title, "SPACE TRIPPER");
	dynmenu_add_item(&menu, "NEW GAME", ST_NEW_GAME, 0);
	dynmenu_add_item(&menu, "QUIT", ST_EXIT, 0);
	menu.menu_active = 1;
	st_program_state = ST_DRAW_MENU;
}

static void st_draw_menu(void)
{
	dynmenu_draw(&menu);
	gs.last_screen = UNKNOWN_SCREEN;
	st_program_state = ST_RENDER_SCREEN;
}

static void st_game_init(void)
{
	dynmenu_init(&menu, menu_item, 20);
	FbInit();
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();
	FbSwapBuffers();
	setup_main_menu();
	st_draw_menu();
	st_program_state = ST_RENDER_SCREEN;
	gs.srs_needs_update = 0;
	gs.last_screen = 255;
	gs.last_capn_menu_item = 0;
	memset(&gs.weapon, 0, sizeof(gs.weapon));
}

static void st_captain_menu(void)
{
	clear_menu();
	strcpy(menu.title, "USS ENTERPRISE"); /* <-- If you change this line, you also must change button_pressed() */
	strcpy(menu.title3, "CAPN'S ORDERS?");
	dynmenu_add_item(&menu, "SHORT RNG SCAN", ST_SRS, 0);
	dynmenu_add_item(&menu, "LONG RNG SCAN", ST_LRS, 0);
	dynmenu_add_item(&menu, "STAR CHART", ST_STAR_CHART, 0);
	dynmenu_add_item(&menu, "SET COURSE", ST_SET_COURSE, 0);
	dynmenu_add_item(&menu, "WARP CTRL", ST_WARP, 0);
	dynmenu_add_item(&menu, "WEAPONS CTRL", ST_AIM_WEAPONS, 0);
	dynmenu_add_item(&menu, "SHIELD CTRL", ST_SHIELD_CONTROL, 0);
	dynmenu_add_item(&menu, "DAMAGE REPORT", ST_DAMAGE_REPORT, 0);
	dynmenu_add_item(&menu, "STATUS REPORT", ST_STATUS_REPORT, 0);
	dynmenu_add_item(&menu, "SENSORS", ST_SENSORS, 0);
	dynmenu_add_item(&menu, "PLANETS", ST_PLANETS, 0);
	dynmenu_add_item(&menu, "STANDARD ORBIT", ST_STANDARD_ORBIT, 0);
	dynmenu_add_item(&menu, "DOCKING CTRL", ST_DOCK, 0);
	dynmenu_add_item(&menu, "TRANSPORTER", ST_TRANSPORTER, 0);
	dynmenu_add_item(&menu, "MINE DILITHIUM", ST_MINE_DILITHIUM, 0);
	dynmenu_add_item(&menu, "LOAD DILITHIUM", ST_LOAD_DILITHIUM, 0);
	dynmenu_add_item(&menu, "SELF DESTRUCT", ST_SELF_DESTRUCT_CONFIRM, 0);
	dynmenu_add_item(&menu, "NEW GAME", ST_NEW_GAME, 0);
	dynmenu_add_item(&menu, "QUIT GAME", ST_EXIT, 0);
	menu.current_item = gs.last_capn_menu_item;
	menu.menu_active = 1;
	st_draw_menu();
	FbSwapBuffers();
	gs.last_screen = CAPN_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static int random_coordinate(void)
{
	return xorshift(&xorshift_state) & 0x0007ffff;
}

static void planet_customizer(int i)
{
	gs.object[i].tsd.planet.flags = xorshift(&xorshift_state) & 0xff;
	gs.object[i].tsd.planet.flags &= ~PLANET_KNOWN; /* initially unknown */
	gs.object[i].tsd.planet.flags &= ~PLANET_SCANNED; /* initially unscanned */
}

static void star_customizer(int i)
{
	gs.object[i].tsd.star.class = xorshift(&xorshift_state) & 0xff;
}

typedef void (*object_customizer)(int index);
static void add_simple_objects(int count, unsigned char type, object_customizer customize)
{
	int i, k;

	for (i = 0; i < count; i++) {
		k = add_object(type, random_coordinate(), random_coordinate());
		if (customize && k != -1)
			customize(k);
	}
}

static void add_enemy_ships(int count, unsigned char shiptype, unsigned char hitpoints)
{
	int i, k;

	for (i = 0; i < count; i++) {
		k = add_object(ENEMY_SHIP, random_coordinate(), random_coordinate());
		gs.object[k].tsd.ship.shiptype = shiptype;
		gs.object[k].tsd.ship.hitpoints = hitpoints;
	}
}

static void init_player()
{
	int i;

	gs.player.x = random_coordinate();
	gs.player.y = random_coordinate();
	gs.player.heading = 0;
	gs.player.new_heading = 0;
	gs.player.energy = INITIAL_ENERGY;
	gs.player.shields = 255;
	gs.player.shields_up = 1;
	gs.player.dilithium_crystals = INITIAL_DILITHIUM;
	gs.player.warp_factor = 0;
	gs.player.new_warp_factor = 0;
	gs.player.torpedoes = INITIAL_TORPEDOES;
	gs.player.phaser_power = 0;
	gs.player.new_phaser_power = 0;
	gs.player.docked = 0;
	gs.player.away_teams_crystals = 0;
	gs.player.mined_dilithium = 0;
#define ABOARD_SHIP 255
	gs.player.away_team = ABOARD_SHIP;
	gs.player.life_support_reserves = 255;
	gs.player.alive = 1;

	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++)
		gs.player.damage[i] = 0;
}

static void st_new_game(void)
{
	memset(&gs, 0, sizeof(gs));
	add_enemy_ships(NKLINGONS, 'K', 100);
	add_enemy_ships(NCOMMANDERS, 'C', 200);
	add_enemy_ships(NROMULANS, 'R', 250);
	add_simple_objects(NBLACKHOLES, BLACKHOLE, NULL);
	add_simple_objects(NSTARBASES, STARBASE, NULL);
	add_simple_objects(NSTARS, STAR, star_customizer);
	add_simple_objects(NPLANETS, PLANET, planet_customizer);
	init_player();
	gs.stardate = 0;
	gs.enddate = gs.stardate + 30 * TICKS_PER_DAY;
	gs.score = 0;
	gs.game_over = 0;
	gs.last_capn_menu_item = 0;
	memset(&gs.visited_sectors, 0, sizeof(gs.visited_sectors));
	st_program_state = ST_CAPTAIN_MENU;
}

static void st_render_screen(void)
{
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void button_pressed()
{
	if (gs.game_over) {
		gs.game_over = 0;
		st_program_state = ST_GAME_INIT;
		return;
	}
	if (!menu.menu_active) {
		menu.menu_active = 1;
	} else {
		st_program_state = menu.item[menu.current_item].next_state;
		menu.menu_active = 0;
		/* hacky way to tell if the current menu is the capn menu */
		if (menu.title[0] == 'U' && menu.title[1] == 'S' && menu.title[2] == 'S')
			gs.last_capn_menu_item = menu.current_item; /* Remember where we are on capn menu */
		FbClear();
	}
}

static void strcatnum(char *s, int n)
{
	char num[10];
	sprintf( num, "%d", n);
	strcat(s, num);
}

static void print_current_sector(void)
{
	char msg[40];

	strcpy(msg, "SECTOR: (");
	strcatnum(msg, coord_to_sector(gs.player.x));
	strcat(msg, ",");
	strcatnum(msg, coord_to_sector(gs.player.y));
	strcat(msg, ")");
	FbWriteLine(msg);
}

static void print_current_quadrant(void)
{
	char msg[40];

	strcpy(msg, "QUADRANT: (");
	strcatnum(msg, coord_to_quadrant(gs.player.x));
	strcat(msg, ",");
	strcatnum(msg, coord_to_quadrant(gs.player.x));
	strcat(msg, ")");
	FbWriteLine(msg);
}

static void print_current_heading(void)
{
	char msg[40];
	strcpy(msg, "BEARING: ");
	strcatnum(msg, gs.player.heading);
	FbWriteLine(msg);
}

static void print_sector_quadrant_heading(void)
{
	FbColor(GREEN);
	FbMove(2, 10);
	print_current_sector();
	FbMove(2, 19);
	print_current_quadrant();
	FbMove(2, 28);
	print_current_heading();
}

static void print_speed(char *impulse, char *warp, int x, int y, int warp_f)
{
	char msg[10];

	FbMove(x, y);
	if (warp_f < WARP1) {
		FbWriteLine(impulse);
		strcpy(msg, "0.");
		strcatnum(msg, impulse_factor(warp_f));
	} else {
		FbWriteLine(warp);
		strcpy(msg, "");
		strcatnum(msg, warp_factor(warp_f));
		strcat(msg, ".");
		strcatnum(msg, warp_factor_frac(warp_f));
	}
	FbMove(x, y + 9);
	FbWriteLine(msg);
}

static void print_power(int x, int y, int power)
{
	char msg[10];

	FbMove(x, y);
	FbWriteLine("POWER");
	strcpy(msg, "");
	strcatnum(msg, warp_factor(power));
	strcat(msg, ".");
	strcatnum(msg, warp_factor_frac(power));
	FbMove(x, y + 9);
	FbWriteLine(msg);
}

static void single_digit_to_dec(int digit, char *num)
{
	if (digit < 0) {
		num[0] = '-';
		num[1] = -digit + '0';
		num[2] = '\0';
	} else {
		num[0] = digit + '0';
		num[1] = '\0';
	}
}

static void screen_header(char *title)
{

	FbClear();
	FbMove(2, 2);
	FbColor(WHITE);
	FbWriteLine(title);
}

static void st_lrs(void) /* long range scanner */
{
	int i, x, y, place;
	int sectorx, sectory, sx, sy;
	char scan[3][3][3];
	const int color[] = { WHITE, CYAN, YELLOW };
	char num[4];

	if (gs.player.energy < LRS_ENERGY) {
		alert_player("SENSORS", "CAPTAIN\n\nWE DON'T HAVE\nENOUGH ENERGY\n"
					"FOR THE LONG\nRANGE SCAN");
		return;
	}
	reduce_player_energy(LRS_ENERGY);

	sectorx = coord_to_sector(gs.player.x);
	sectory = coord_to_sector(gs.player.y);
	memset(scan, 0, sizeof(scan));

	/* Count up nearby enemy ships, starbases and stars, and store counts in scan[][][] */
	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case ENEMY_SHIP:
			place = 0;
			break;
		case STARBASE:
			place = 1;
			break;
		case STAR:
			place = 2;
			break;
		default:
			continue;
		}
		sx = coord_to_sector(gs.object[i].x);
		sy = coord_to_sector(gs.object[i].y);
		if (sx > sectorx + 1 || sx < sectorx - 1)
			continue;
		if (sy > sectory + 1 || sy < sectory - 1)
			continue;
		/* Calculate sector relative to player in range -1 - +1, then add 1 to get into range 0 - 2 */
		x = sx - sectorx + 1;
		y = sy - sectory + 1;
		scan[x][y][place]++;
	}

	screen_header("LONG RANGE SCAN");
	print_sector_quadrant_heading();
	print_speed("IMP", "WRP", 95, 28, gs.player.warp_factor);

	FbColor(CYAN);
	for (x = -1; x < 2; x++) { /* Print out X coordinates */
		single_digit_to_dec(x + coord_to_sector(gs.player.x), num);
		FbMove(35 + (x + 1) * 30, 50);
		FbWriteLine(num);
	}

	for (y = -1; y < 2; y++) { /* Print out Y coordinates */
		single_digit_to_dec(y + coord_to_sector(gs.player.y), num);
		FbMove(5, (y + 1) * 10 + 60);
		FbWriteLine(num);
	}

	/* Mark visited sectors */
	for (x = -1; x < 2; x++) {
		sx = sectorx + x;
		if (sx < 0 || sx > 7)
			continue;
		for (y = -1; y < 2; y++) {
			sy = sectory + y;
			if (sy < 0 || sy > 7)
				continue;
			mark_sector_visited((unsigned char) sx, (unsigned char) sy);
		}
	}

	/* Print out scan[][][] */
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			for (i = 0; i < 3; i++) {
				char digit[5];

				FbColor(color[i]);
				sprintf( digit, "%d", scan[x][y][i]);
				FbMove(25 + x * 30 + i * 8, y * 10 + 60);
				FbWriteLine(digit);
			}
		}
		FbColor(GREEN);
		FbHorizontalLine(22, y * 10 + 59, 132 - 21, y * 10 + 59);
	}
	FbHorizontalLine(22, y * 10 + 59, 132 - 21, y * 10 + 59);
	for (i = 0; i < 4; i++)
		FbVerticalLine(22 + i * 30, 59, 22 + i * 30,  y * 10 + 59);

	FbMove(2, 98);
	FbColor(color[0]);
	FbWriteLine("# ENEMY SHIPS");
	FbMove(2, 107);
	FbColor(color[1]);
	FbWriteLine("# STARBASES");
	FbMove(2, 116);
	FbColor(color[2]);
	FbWriteLine("# STARS");
	
	FbSwapBuffers();
	gs.last_screen = LRS_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static void star_chart_question_mark(int sx, int sy)
{
	FbColor(GREEN);
	FbMove(6 + sx * 16, 6 + sy * 16);
	FbWriteLine("?");
}

static void plot_on_star_chart(int sx, int sy, int qx, int qy, int color)
{
	FbColor(color);
	FbPoint(2 + sx * 16 + qx, 2 + sy * 16 + qy);
	FbPoint(3 + sx * 16 + qx, 2 + sy * 16 + qy);
	FbPoint(2 + sx * 16 + qx, 3 + sy * 16 + qy);
	FbPoint(3 + sx * 16 + qx, 3 + sy * 16 + qy);
}

static void st_star_chart(void)
{
	int i, sx, sy, qx, qy, color;

	FbClear();

	for (sx = 0; sx < 8; sx++)
		for (sy = 0; sy < 8; sy++)
			if (!sector_visited(sx, sy))
				star_chart_question_mark(sx, sy);

	sx = coord_to_sector(gs.player.x);
	sy = coord_to_sector(gs.player.y);
	qx = coord_to_quadrant(gs.player.x);
	qy = coord_to_quadrant(gs.player.y);
	plot_on_star_chart(sx, sy, qx, qy, MAGENTA);

	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case 0:
			continue;
		case ENEMY_SHIP:
			color = WHITE;
			break;
		case PLANET:
			color = CYAN;
			break;
		case BLACKHOLE:
			color = RED;
			break;
		case STARBASE:
			color = GREEN;
			break;
		case STAR:
			color = YELLOW;
			break;
		default:
			continue;
		}

		sx = coord_to_sector(gs.object[i].x);
		sy = coord_to_sector(gs.object[i].y);
		if (!sector_visited((unsigned char) sx, (unsigned char) sy))
			continue;
		qx = coord_to_quadrant(gs.object[i].x);
		qy = coord_to_quadrant(gs.object[i].y);
		plot_on_star_chart(sx, sy, qx, qy, color);
	}
	FbSwapBuffers();
	gs.last_screen = STAR_CHART_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static void st_srs(void) /* short range scanner */
{
	int i, x, y;
	int sectorx, sectory, sx, sy, qx, qy;
	char scan[3][3][3];
	char c[2] = { '.', '\0' };
	int color;
	const int quadrant_width = 10;
	char num[4];
	const int left = 16;

	sectorx = coord_to_sector(gs.player.x);
	sectory = coord_to_sector(gs.player.y);
	mark_sector_visited((unsigned char) sectorx, (unsigned char) sectory);
	memset(scan, 0, sizeof(scan));

	screen_header("SHORT RANGE SCAN");
	print_sector_quadrant_heading();
	print_speed("IMP", "WRP", 95, 48, gs.player.warp_factor);

	/* Draw a grid */
	FbColor(BLUE);
	for (y = 0; y < 9; y++)
		FbHorizontalLine(left - 2, 45 + y * quadrant_width, left - 2 + 8 * quadrant_width, 45 + y * quadrant_width);
	for (x = 0; x < 9; x++)
		FbVerticalLine(left - 2 + x * quadrant_width, 45, left - 2 + x * quadrant_width, 45 + 8 * quadrant_width);

	/* Draw X coordinates */
	FbColor(CYAN);
	num[1] = '\0';
	for (x = 0; x < 8; x++) {
		num[0] = '0' + x;
		FbMove(left + x * quadrant_width, 37);
		FbWriteLine(num);
	}
	/* Draw Y coordinates */
	for (y = 0; y < 8; y++) {
		num[0] = '0' + y;
		FbMove(4, 45 + y * quadrant_width);
		FbWriteLine(num);
	}

	/* Find objects in this sector */
	for (i = 0; i < NTOTAL; i++) {

		if (gs.object[i].type == 0)
			continue;

		sx = coord_to_sector(gs.object[i].x);
		sy = coord_to_sector(gs.object[i].y);

		if (sx != sectorx || sy != sectory)
			continue;

		switch (gs.object[i].type) {
		case ENEMY_SHIP:
			color = WHITE;
			c[0] = gs.object[i].tsd.ship.shiptype;
			break;
		case PLANET:
			color = CYAN;
			c[0] = 'O';
			gs.object[i].tsd.planet.flags |= PLANET_KNOWN;
			break;
		case STAR:
			color = YELLOW;
			c[0] = '*';
			break;
		case STARBASE:
			color = GREEN;
			c[0] = 'S';
			break;
		case BLACKHOLE:
			color = RED;
			c[0] = 'B';
			break;
		default:
			color = YELLOW;
			c[0] = '?';
			break;
		}
		qx = coord_to_quadrant(gs.object[i].x);
		qy = coord_to_quadrant(gs.object[i].y);
		FbMove(left + qx * quadrant_width, 47 + qy * quadrant_width);
		FbColor(color);
		FbWriteLine(c);
	}

	/* Draw player */
	qx = coord_to_quadrant(gs.player.x);
	qy = coord_to_quadrant(gs.player.y);
	FbMove(left + qx * quadrant_width, 47 + qy * quadrant_width);
	FbColor(MAGENTA);
	FbWriteLine("E");

	FbSwapBuffers();
	gs.last_screen = SRS_SCREEN;
	gs.srs_needs_update = 0;
	st_program_state = ST_PROCESS_INPUT;
}

static void st_srs_legend(void)
{
	FbClear();
	FbColor(WHITE);
	FbMove(2, 2);
	FbWriteString("SRS LEGEND\n\n");
	FbColor(MAGENTA);
	FbWriteString("E ENTERPRISE\n");
	FbColor(WHITE);
	FbWriteString("K KLINGON\n");
	FbWriteString("C KLINGON CMDR\n");
	FbWriteString("R ROMULAN\n");
	FbColor(GREEN);
	FbWriteString("S STAR BASE\n");
	FbColor(RED);
	FbWriteString("B BLACK HOLE\n");
	FbColor(YELLOW);
	FbWriteString("* STAR\n");
	FbColor(CYAN);
	FbWriteString("O PLANET\n");
	FbSwapBuffers();
	gs.last_screen = SRS_LEGEND_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static void print_numeric_item_with_frac(char *item, int value, int frac)
{
	char num[10];

	FbWriteString(item);
	sprintf( num, "%d", value);
	FbWriteString(num);
	FbWriteString(".");
	sprintf( num, "%d", frac);
	FbWriteString(num);
	FbMoveX(2);
	FbMoveRelative(0, 9);
}

static void print_numeric_item(char *item, int value)
{
	char num[10];

	FbWriteString(item);
	sprintf( num, "%d", value);
	FbWriteString(num);
	FbMoveX(2);
	FbMoveRelative(0, 9);
}

static void write_heading(char *msg, int heading, int color)
{
	FbColor(color);
	print_numeric_item(msg, heading);
}

static void draw_heading_indicator(const int cx, const int cy, const int degrees, const int length, const int color)
{
	int a, x, y;

	FbColor(color);
	a = (degrees * 128) / 360;
	x = cx + (cosine(a) * length) / 1024;
	y = cy + (-sine(a) * length) / 1024;
	FbLine(cx, cy, x, y);
}

static void show_energy_and_torps(void)
{
	FbMove(2, 20);
	print_numeric_item("ENERGY:", gs.player.energy);
	print_numeric_item("TORP:", gs.player.torpedoes);
}

static void st_choose_angle_input(void)
{
	int down_latches = button_down_latches();
	int rotary = button_get_rotation(0);
	int something_changed = 0;

	gs.angle_chooser.old_angle = *gs.angle_chooser.angle;
	gs.angle_chooser.old_new_angle = *gs.angle_chooser.new_angle;

	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		int angle = *gs.angle_chooser.new_angle;
		*gs.angle_chooser.angle = angle;
		gs.angle_chooser.old_angle = angle;
		gs.angle_chooser.old_new_angle = angle;
		gs.angle_chooser.finished = 1;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0) {
		*gs.angle_chooser.new_angle += 15;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0) {
		*gs.angle_chooser.new_angle -= 15;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		(*gs.angle_chooser.new_angle)++;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		(*gs.angle_chooser.new_angle)--;
		something_changed = 1;
	}
	if (something_changed) {
		if (*gs.angle_chooser.new_angle < 0)
			*gs.angle_chooser.new_angle += 360;
		if (*gs.angle_chooser.new_angle >= 360)
			*gs.angle_chooser.new_angle -= 360;
		st_program_state = ST_DRAW_ANGLE_CHOOSER;
	}
}

static void st_draw_angle_chooser(void)
{
	int i, x, y;
	const int cx = (LCD_XSIZE >> 1);
	const int cy = (LCD_YSIZE >> 1) + 15;
	struct angle_chooser_t *a = &gs.angle_chooser;

	/* Erase old stuff */
	FbMove(2, 20);
	write_heading(a->new_angle_name, a->old_new_angle, BLACK);
	FbMove(2, 11);
	write_heading(a->cur_angle_name, a->old_angle, BLACK);
	if (gs.angle_chooser.show_extra_data)
		gs.angle_chooser.show_extra_data();
	draw_heading_indicator(cx, cy, a->old_angle, 150, BLACK);
	draw_heading_indicator(cx, cy, a->old_new_angle, 150, BLACK);

	/* Draw new stuff */
	if (!a->finished) {
		FbColor(WHITE);
		FbMove(2, 2);
		FbWriteLine(a->set_angle_name);
		FbMove(2, 20);
		write_heading(a->new_angle_name, *a->new_angle, WHITE);
	}
	FbMove(2, 11);
	write_heading(a->cur_angle_name, *a->angle, WHITE);

	draw_heading_indicator(cx, cy, *a->angle, 150, RED);
	draw_heading_indicator(cx, cy, *a->new_angle, 150, WHITE);
	/* Draw a circle of dots. */
	for (i = 0; i < 128; i += 4) {
		x = (-cosine(i) * 160) / 1024;
		y = (sine(i) * 160) / 1024;
		FbPoint(cx + x, cy + y);
	}
	if (gs.angle_chooser.show_extra_data)
		gs.angle_chooser.show_extra_data();
	FbSwapBuffers();
	if (gs.angle_chooser.finished) {
		if (gs.angle_chooser.angle == &gs.player.weapons_aim)
			st_program_state = ST_CHOOSE_WEAPONS;
		else
			st_program_state = ST_CAPTAIN_MENU; 
		return;
	}
	st_program_state = ST_CHOOSE_ANGLE_INPUT;
}

static void st_choose_angle(char *new_head, char *cur_head, char *set_head,
		int *heading, int *new_heading, int which_screen, void (*show_extra_data)(void))
{
	gs.angle_chooser.angle = heading;
	gs.angle_chooser.new_angle = new_heading;
	gs.angle_chooser.old_angle = *heading;
	gs.angle_chooser.old_new_angle = *new_heading;
	gs.angle_chooser.new_angle_name = new_head;
	gs.angle_chooser.cur_angle_name = cur_head;
	gs.angle_chooser.set_angle_name = set_head;
	gs.angle_chooser.show_extra_data = show_extra_data;
	
	gs.last_screen = which_screen;
	st_program_state = ST_DRAW_ANGLE_CHOOSER;
}

static void st_set_course(void)
{
	int old_heading = gs.player.heading;
	int energy_units, angle, warp_multiplier;

	gs.angle_chooser.finished = 0;
	st_choose_angle("NEW HEADING: ", "CUR HEADING", "SET HEADING: ",
			&gs.player.heading, &gs.player.new_heading, HEADING_SCREEN, NULL);

	angle = abs(gs.player.heading - old_heading);
	if (angle > 180)
		angle = 360 - angle;
	warp_multiplier = gs.player.warp_factor / WARP1;
	energy_units = (STEERING_ENERGY_PER_DEG * angle * warp_multiplier) / 100;

	if (energy_units > gs.player.energy) {
		alert_player("NAVIGATION", "CAPTAIN\n\nTHERE IS NOT\nENOUGH\nENERGY TO\nCHANGE COURSE");
		gs.player.heading = old_heading;
		return;
	}
	reduce_player_energy(energy_units);
}

static void st_choose_weapons(void)
{
	clear_menu();	
	strcpy(menu.title, "CHOOSE WEAPON");
	dynmenu_add_item(&menu, "PHOTON TORPS", ST_PHOTON_TORPEDOES, 0);
	dynmenu_add_item(&menu, "PHASER BEAMS", ST_PHASER_BEAMS, 0);
	dynmenu_add_item(&menu, "CANCEL WEAPONS", ST_CAPTAIN_MENU, 0);
	menu.menu_active = 1;
	st_program_state = ST_DRAW_MENU;
}

static void st_process_input(void)
{
    int down_latches = button_down_latches();
    int rotary = button_get_rotation(0);
    int something_happened = 0;
    if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
        button_pressed();
	something_happened = 1;
    } else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0) {
        if (menu.menu_active)
            dynmenu_change_current_selection(&menu, -1);
	something_happened = 1;
    } else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0) {
        if (menu.menu_active)
            dynmenu_change_current_selection(&menu, 1);
	something_happened = 1;
    } else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
        if (gs.last_screen == SRS_SCREEN) {
            st_program_state = ST_SRS_LEGEND;
	    return;
	}
        if (gs.last_screen == SRS_LEGEND_SCREEN) {
            st_program_state = ST_SRS;
            return;
        }
    } else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
        if (gs.last_screen == SRS_SCREEN) {
            st_program_state = ST_SRS_LEGEND;
	    return;
	}
        if (gs.last_screen == SRS_LEGEND_SCREEN) {
            st_program_state = ST_SRS;
            return;
        }
    }
    if (st_program_state == ST_PROCESS_INPUT && something_happened) {
        if (menu.menu_active)
            st_program_state = ST_DRAW_MENU;
    }
}

static void strcat_sector_quadrant(char *msg, int x, int y)
{
	int sx, sy, qx, qy;

	sx = coord_to_sector(x);
	sy = coord_to_sector(y);
	qx = coord_to_quadrant(x);
	qy = coord_to_quadrant(y);
	strcat(msg, "S(");
	strcatnum(msg, sx);
	strcat(msg, ",");
	strcatnum(msg, sy);
	strcat(msg, ")Q(");
	strcatnum(msg, qx);
	strcat(msg, ",");
	strcatnum(msg, qy);
	strcat(msg, ")");
}

static void print_planets_report(int i, int n, int x, int y, int scanned)
{
	char msg[40];
	char cl[] = "CLASS  ";
	int c;

	strcpy(msg, "P");
	strcatnum(msg, n);
	strcat(msg, " ");
	strcat_sector_quadrant(msg, x, y);
	FbWriteLine(msg);
	FbMoveX(2);
	FbMoveRelative(0, 10);
	strcpy(msg, "");
	if (!scanned) {
		strcat(msg, " - NOT SCANNED");
	} else {
		if (PLANET_INHABITED & gs.object[i].tsd.planet.flags)
			strcat(msg, " I");
		if (PLANET_HAS_DILITHIUM & gs.object[i].tsd.planet.flags)
			strcat(msg, " DC ");
		c = PLANET_CLASS(gs.object[i].tsd.planet.flags);
		cl[6] = planet_class[c];
		strcat(msg, cl);
	}
	FbWriteLine(msg);
}

static void st_planets(void)
{
	int i, count, scanned;

	screen_header("PLANETS:");
	FbColor(GREEN);
	count = 0;

	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case PLANET:
			if (gs.object[i].tsd.planet.flags & PLANET_KNOWN) {
				count++;
				scanned = gs.object[i].tsd.planet.flags & PLANET_SCANNED;
				FbMove(2, 20 * (count - 1) + 10);
				print_planets_report(i, count, gs.object[i].x, gs.object[i].y, scanned);
			}
			break;
		default:
			break;
		}
	}
	FbColor(WHITE);
	FbMove(2, (count) * 20 + 1);
	print_numeric_item("TOTAL COUNT: ", count);
	FbSwapBuffers();
	gs.last_screen = PLANETS_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static int player_is_next_to(unsigned char object_type)
{
	int i, sx, sy, qx, qy;

	/* See if there is an object of the specified type nearby */
	sx = coord_to_sector(gs.player.x);
	sy = coord_to_sector(gs.player.y);
	qx = coord_to_quadrant(gs.player.x);
	qy = coord_to_quadrant(gs.player.y);
	for (i = 0; i < NTOTAL; i++) {
		struct game_object *o = &gs.object[i];
		if (o->type != object_type)
			continue;
		if (sx != coord_to_sector(o->x))
			continue;
		if (sy != coord_to_sector(o->y))
			continue;
		if (abs(qx - coord_to_quadrant(o->x) > 1))
			continue;
		if (abs(qy - coord_to_quadrant(o->y) > 1))
			continue;
		return i + 1; /* Add 1 so that we never return 0 here and result can be used as boolean */
	}
	return 0;
}

static int object_is_next_to_player(int object)
{
	if (coord_to_sector(gs.player.x) != coord_to_sector(gs.object[object].x))
		return 0;
	if (coord_to_sector(gs.player.y) != coord_to_sector(gs.object[object].y))
		return 0;
	if (abs(coord_to_quadrant(gs.player.x) - coord_to_quadrant(gs.object[object].x)) > 1)
		return 0;
	if (abs(coord_to_quadrant(gs.player.y) - coord_to_quadrant(gs.object[object].y)) > 1)
		return 0;
	return 1;
}

static void st_dock(void)
{
	/* If player docked, undock player */
	if (gs.player.docked) {
		gs.player.docked = 0;
		alert_player("DOCKING CONTROL", "CAPTAIN THE\nSHIP HAS BEEN\nUNDOCKED FROM\nSTARBASE");
		return;
	}

	/* See if there is a starbase nearby */
	if (player_is_next_to(STARBASE)) {
		alert_player("DOCKING CONTROL", "CAPTAIN THE\nSHIP HAS BEEN\nDOCKED WITH\nSTARBASE\n\nSUPPLIES\nREPLENISHING");
		gs.player.docked = 1;
	} else {
		alert_player("DOCKING CONTROL", "CAPTAIN THERE\nARE NO NEARBY\nSTARBASES WITH\nWHICH TO DOCK");
	}

	/* Ship's supplies get replenished in move_player() */
}

static void st_standard_orbit(void)
{
	if (gs.player.docked) {
		alert_player("NAVIGATION", "CAPTAIN THE\nSHIP CANNOT\nENTER STANDARD\nORBIT WHILE\nDOCKED WITH\nTHE STARBASE");
		return;
	}

	if (gs.player.standard_orbit) {
		gs.player.standard_orbit = 0;
		alert_player("NAVIGATION", "CAPTAIN WE\nHAVE LEFT\nSTANDARD ORBIT\nAND ARE IN\nOPEN SPACE");
		return;
	}

	if (gs.player.warp_factor > 0) {
		alert_player("NAVIGATION", "CAPTAIN\n\nWE MUST\nCOME OUT OF\nWARP BEFORE\nENTERING\nSTANDARD ORBIT");
		return;
	}

	if (player_is_next_to(PLANET)) {
		gs.player.standard_orbit = 1;
		gs.player.warp_factor = 0; /* belt and suspenders */
		alert_player("NAVIGATION", "CAPTAIN WE\nHAVE ENTERED\nSTANDARD ORBIT\nAROUND THE\nPLANET");
	} else {
		alert_player("NAVIGATION", "CAPTAIN THERE\nIS NO PLANET\nCLOSE ENOUGH\nTO ENTER\nSTANDARD ORBIT");
	}
}

static void st_transporter(void)
{
	int planet;

	if (!gs.player.standard_orbit) {
		alert_player("TRANSPORTER", "CAPTAIN\n\nWE ARE NOT\nCURRENTLY IN\nORBIT AROUND\nA SUITABLE\nPLANET");
		return;
	}

	if (gs.player.energy < TRANSPORTER_ENERGY) {
		alert_player("TRANSPORTER", "CAPTAIN\n\nTHERE IS NOT\nENOUGH\n"
						"ENERGY TO\nUSE THE\nTRANSPORTER\n");
		return;
	}

	if (gs.player.away_team != ABOARD_SHIP) {
		if (object_is_next_to_player(gs.player.away_team)) {
			gs.player.away_team = ABOARD_SHIP;
			reduce_player_energy(TRANSPORTER_ENERGY);
			if (gs.player.away_teams_crystals > 0) {
				int dilith_crystals;

				alert_player("TRANSPORTER", "CAPTAIN\n\nAWAY TEAM\nHAS BEAMED\nABOARD FROM\n"
							"PLANET SURFACE\nWITH DILITHIUM\nCRYSTALS");
				dilith_crystals = gs.player.mined_dilithium + gs.player.away_teams_crystals;
				if (dilith_crystals > 255)
					dilith_crystals = 255;
				gs.player.mined_dilithium = dilith_crystals;
				gs.player.away_teams_crystals = 0;
			} else {
				alert_player("TRANSPORTER", "CAPTAIN\n\nAWAY TEAM\nHAS BEAMED\nABOARD FROM\nPLANET SURFACE");
			}
		} else {
			planet = player_is_next_to(PLANET);
			if (!planet) /* Shouldn't happen as we're in standard orbit, therefore next to a planet. */
				alert_player("TRANSPORTER", "CAPTAIN\n\nTHE PLANET THE\nAWAY TEAM IS\nON IS NOT\nNEARBY");
			else
				alert_player("TRANSPORTER", "CAPTAIN\n\nUNABLE TO\nOBTAIN A LOCK\nON THE AWAY\nTEAM.\n"
								"I THINK THEY\nARE ON ANOTHER\nPLANET");
		}
		return;
	}

	planet = player_is_next_to(PLANET);
	if (!planet) {
		/* This is a bug. We should never be in standard orbit while not next to a planet. */
		alert_player("TRANSPORTER", "CAPTAIN\n\nSOMETHING IS\nWRONG. I CAN'T\nGET A LOCK ON\nTHE PLANET");
		return;
	}
	/* Remember on which planet we dropped off the away team. */
	gs.player.away_team = planet - 1; /* player_is_next_to() added 1 so value can be used as boolean, so we subtract here. */
	alert_player("TRANSPORTER", "CAPTAIN\n\nAWAY TEAM\nHAS BEAMED\nDOWN TO THE\nPLANET SURFACE");
	reduce_player_energy(TRANSPORTER_ENERGY);
}

static void st_mine_dilithium(void)
{
	struct game_object *planet;

	if (gs.player.away_team == ABOARD_SHIP) {
		alert_player("COMMS", "CAPTAIN\n\nTHE AWAY TEAM\nIS STILL ABOARD\nTHE SHIP");
		return;
	}
	planet = &gs.object[gs.player.away_team];
	if (planet->tsd.planet.flags & PLANET_HAS_DILITHIUM) {
		alert_player("COMMS", "CAPTAIN\n\nTHE AWAY\nTEAM REPORTS\nTHEY HAVE\n"
				"FOUND\nDILITHIUM\nCRYSTALS");
		gs.player.away_teams_crystals += 5 + (xorshift(&xorshift_state) & 0x07);
		if (gs.player.away_teams_crystals > 50)
			gs.player.away_teams_crystals = 50;
		return;
	}
	alert_player("COMMS", "CAPTAIN\n\nTHE AWAY\nTEAM REPORTS\nTHEY HAVE\n"
			"FOUND NO\nDILITHIUM\nCRYSTALS");
}

static void st_load_dilithium(void)
{
	int n, leftover = 0;

	if (gs.player.mined_dilithium == 0) {
		alert_player("ENGINEERING", "CAPTAIN\n\nWE HAVEN'T GOT\nANY EXTRA\nDILITHIUM\nCRYSTALS\nTO LOAD INTO\nTHE WARP CHAMBER");
		return;
	}
	if (gs.player.dilithium_crystals == 255) {
		alert_player("ENGINEERING", "CAPTAIN\n\nTHE WARP DRIVE\nIS AT FULL\nCAPACITY ALREADY");
		return;
	}
	n = gs.player.dilithium_crystals + gs.player.mined_dilithium;
	if (n > 255) {
		n = 255;
		leftover = gs.player.dilithium_crystals + gs.player.mined_dilithium - 255;
	}
	gs.player.dilithium_crystals = n;
	gs.player.mined_dilithium = leftover;
	alert_player("ENGINEERING", "CAPTAIN\n\nWE HAVE LOADED\nUP THE MINED\n"
				"DILITHIUM\nCRYSTALS\nINTO THE WARP\nCHAMBER\n"
				"HOPEFULLY THE\nQUALITY IS NOT\nTOO POOR");
}

static void st_self_destruct_confirm(void)
{
	clear_menu();
	strcpy(menu.title, "SELF DESTRUCT?");
	dynmenu_add_item(&menu, "NO DO NOT DESTRUCT", ST_CAPTAIN_MENU, 0);
	dynmenu_add_item(&menu, "YES SELF DESTRUCT", ST_SELF_DESTRUCT, 0);
	menu.menu_active = 1;
	st_program_state = ST_DRAW_MENU;
}

static void adjust_score_on_kill(int target)
{
	switch (gs.object[target].tsd.ship.shiptype) {
	case 'K':
		gs.score += KLINGON_POINTS;
		break;
	case 'C':
		gs.score += COMMANDER_POINTS;
		break;
	case 'R':
		gs.score += ROMULAN_POINTS;
		break;
	default:
		break;
	}
}

static void st_self_destruct(void)
{
	int i;
	char msg[60];

	/* Kill any enemy ships in the sector. */
	for (i = 0; i < NTOTAL; i++) {
		struct game_object *o = &gs.object[i];
		if (o->type != ENEMY_SHIP)
			continue;
		if (coord_to_sector(o->x) != coord_to_sector(gs.player.x))
			continue;
		if (coord_to_sector(o->y) != coord_to_sector(gs.player.y))
			continue;
		adjust_score_on_kill(i);
		delete_object(i);
	}
	strcpy(msg, "YOUR FINAL\nSCORE WAS: ");
	strcatnum(msg, gs.score);
	alert_player("GAME OVER", msg);
	gs.game_over = 1;
}

static void st_sensors(void)
{
	int i, sx, sy, px, py;
	int candidate = -1;
	int mindist = 10000000;
	int dist;

	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case PLANET:
			sx = coord_to_sector(gs.object[i].x);
			sy = coord_to_sector(gs.object[i].y);
			px = coord_to_sector(gs.player.x);
			py = coord_to_sector(gs.player.y);
			if (sx != px || sy != py)
				break;
			sx = coord_to_quadrant(gs.player.x);
			sy = coord_to_quadrant(gs.player.y);
			px = coord_to_quadrant(gs.object[i].x);
			py = coord_to_quadrant(gs.object[i].y);
			sx = px - sx;
			sy = py - sy;
			dist = sx * sx + sy * sy;
			if (candidate == -1 || dist < mindist) {
				mindist = dist;
				candidate = i;
			}
			break;
		default:
			break;
		}
	}
	FbClear();
	FbMove(2, 2);
	FbColor(WHITE);
	if (candidate == -1) {
		FbWriteLine("NO NEARBY PLANETS");
		FbMoveX(2);
		FbMoveRelative(0, 10);
		FbWriteLine("TO SCAN");
	} else {
		char c, cl[] = "CLASS  ";
		char msg[40];
		char *flags = &gs.object[candidate].tsd.planet.flags;
		*flags |= PLANET_SCANNED;
		*flags |= PLANET_KNOWN;
		FbWriteLine("NEAREST PLANET:"); FbMoveX(2); FbMoveRelative(0, 10);
		strcpy(msg, "");
		strcat_sector_quadrant(msg, gs.object[candidate].x, gs.object[candidate].y);
		FbWriteLine(msg);
		FbMoveX(2); FbMoveRelative(0, 10);
		c = PLANET_CLASS(*flags);
		cl[6] = planet_class[(int) c];
		FbWriteLine(cl); FbMoveX(2); FbMoveRelative(0, 10);
		FbWriteLine(planet_class_name[(int) c]); FbMoveX(2); FbMoveRelative(0, 10);
		if (*flags & PLANET_INHABITED) {
			FbWriteLine("INHABITED"); FbMoveX(2); FbMoveRelative(0, 10);
		} else {
			FbWriteLine("UNINHABITED"); FbMoveX(2); FbMoveRelative(0, 10);
		}
		if (*flags & PLANET_HAS_DILITHIUM) {
			FbWriteLine("DILITHIUM FOUND"); FbMoveX(2); FbMoveRelative(0, 10);
		} else {
			FbWriteLine("NO DILITHIUM"); FbMoveX(2); FbMoveRelative(0, 10);
		}
		
	}
	FbSwapBuffers();
	gs.last_screen = SENSORS_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static void show_torps_energy_and_dilith(void)
{
	FbMoveRelative(0, 9);
	FbMoveX(2);
	print_numeric_item("ENERGY:", gs.player.energy);
	print_numeric_item("TORPEDOES:", gs.player.torpedoes);
	print_numeric_item("DILITH:", gs.player.dilithium_crystals);
}

static void st_damage_report(void)
{
	int i, d;
	char ds[10];

	screen_header("DAMAGE REPORT");

	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++) {
		FbColor(CYAN);
		FbMove(2, 9 + i * 9);
		FbWriteLine((char *) ship_system[i]);
		d = ((((256 - gs.player.damage[i]) * 1024) / 256) * 100) / 1024;
		if (d < 70)
			FbColor(YELLOW);
		else if (d < 40)
			FbColor(RED);
		else
			FbColor(GREEN);
		sprintf( ds, "%d", d);
		FbMove(LCD_XSIZE - 9 * 4, 9 + i * 9);
		FbWriteLine(ds);
	}
	show_torps_energy_and_dilith();
	FbColor(WHITE);
	if (gs.player.docked)
		FbWriteString("CURRENTLY\nDOCKED AT\nSTARBASE");
	if (gs.player.standard_orbit)
		FbWriteString("CURRENTLY\nIN STANDARD\nORBIT\n");
	if (gs.player.away_team != ABOARD_SHIP)
		FbWriteString("AWAY TEAM OUT");
	FbSwapBuffers();
	gs.last_screen = DAMAGE_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static void st_status_report(void)
{
	int es, sb, i, sd, frac;

	screen_header("STATUS REPORT");
	FbColor(CYAN);

	/* Compute and print current star date */
	FbMove(2, 2 * 9);
	sd = (gs.stardate / 256) + START_DATE;
	frac = ((gs.stardate % 256) * 100) / 256;
	print_numeric_item_with_frac("STARDATE:", sd, frac);

	/* Compute and print time remaining */
	sd = (gs.enddate - gs.stardate) / TICKS_PER_DAY;
	frac = (((gs.enddate - gs.stardate) % TICKS_PER_DAY) * 100) / 256;
	print_numeric_item_with_frac("DAYS LEFT:", sd, frac);

	/* Count remaining starbases and enemy ships */
	es = 0;
	sb = 0;
	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case ENEMY_SHIP:
			es++;
			break;
		case STARBASE:
			sb++;
			break;
		default:
			break;
		}
	}
	print_numeric_item("STARBASES", sb);
	print_numeric_item("ENEMY SHP", es);
	show_torps_energy_and_dilith();
	FbColor(YELLOW);
	print_numeric_item("SCORE:", gs.score);
	FbColor(WHITE);
	if (gs.player.docked)
		FbWriteString("CURRENTLY\nDOCKED AT\nSTARBASE");
	if (gs.player.standard_orbit)
		FbWriteString("CURRENTLY\nIN STANDARD\nORBIT\n");
	if (gs.player.away_team != ABOARD_SHIP)
		FbWriteString("AWAY TEAM OUT");

	FbSwapBuffers();
	gs.last_screen = STATUS_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static int keep_in_bounds(int *coord)
{
	if (*coord < 0) {
		*coord = 0;
		return 1;
	}
	if (*coord > 0x0007ffff) {
		*coord = 0x0007ffff;
		return 1;
	}
	return 0;
}

/* returns true if collision */
static int player_collision_detection(int *nx, int *ny)
{
	int i, sx, sy, qx, qy, osx, osy, oqx, oqy, braking_energy;
	unsigned char neutral_zone;
	char msg[40];

	/* Keep the player in bounds */
	neutral_zone = 0;
	neutral_zone |= keep_in_bounds(nx);
	neutral_zone |= keep_in_bounds(ny);
	braking_energy = 0;
	if (neutral_zone) {
		if (gs.player.warp_factor != 0) {
			braking_energy = (gs.player.warp_factor / WARP1) * WARP_ENERGY_PER_SEC / 2;
			gs.player.warp_factor = 0;
		}
		gs.srs_needs_update = 1;
		alert_player("STARFLEET MSG",
			"PERMISSION TO\nENTER NEUTRAL\nZONE DENIED\n\n"
			"SHUT DOWN\nWARP DRIVE\n\n-- STARFLEET");
		reduce_player_energy(braking_energy);
		return 0;
	}

	sx = coord_to_sector(*nx);
	sy = coord_to_sector(*ny);
	qx = coord_to_quadrant(*nx);
	qy = coord_to_quadrant(*ny);

	for (i = 0; i < NTOTAL; i++) {
		if (gs.object[i].type == 0)
			continue;
		osx = coord_to_sector(gs.object[i].x);
		if (osx != sx)
			continue;
		osy = coord_to_sector(gs.object[i].y);
		if (osy != sy)
			continue;
		oqx = coord_to_quadrant(gs.object[i].x);
		if (oqx != qx)
			continue;
		oqy = coord_to_quadrant(gs.object[i].y);
		if (oqy != qy)
			continue;
		const char *object = object_type_name(gs.object[i].type);
		strcpy(msg, "WE HAVE\nENCOUNTERED\n");
		strcat(msg, object);
		braking_energy = (gs.player.warp_factor / WARP1) * WARP_ENERGY_PER_SEC / 2;
		reduce_player_energy(braking_energy);
		gs.player.warp_factor = 0;
		alert_player("WARP SHUTDOWN", msg);
		reduce_player_energy(braking_energy);
		return 1;
	}
	return 0;
}

static void replenish_int_supply(int *supply, int limit, int increment)
{
	if (*supply < limit) {
		*supply += increment;
		if (*supply > limit)
			*supply = limit;
	}
}

static void replenish_char_supply(unsigned char *supply, unsigned int limit, unsigned char increment)
{
	int v;

	if (*supply < limit) {
		v = *supply + increment;
		if (v > (int) limit)
			v = limit;
		*supply = (unsigned char) v;;
	}
}

static void replenish_supplies_and_repair_ship(void)
{
	int i, n;

	replenish_char_supply(&gs.player.torpedoes, INITIAL_TORPEDOES, 1);
	replenish_int_supply(&gs.player.energy, INITIAL_ENERGY, INITIAL_ENERGY / 10);
	replenish_char_supply(&gs.player.dilithium_crystals, INITIAL_DILITHIUM, INITIAL_DILITHIUM / 10);
	replenish_char_supply(&gs.player.shields, MAX_SHIELD_ENERGY, MAX_SHIELD_ENERGY / 10);
	replenish_char_supply(&gs.player.life_support_reserves, 255, 25);
	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++) { /* Repair damaged systems, including hull */
		if (gs.player.damage[i] > 0) {
			n = gs.player.damage[i] - 25;
			if (n < 0)
				n = 0; 
			gs.player.damage[i] = n;
		}
	}
}

static void conduct_repairs()
{
	static int repair_time = 0;
	char msg[60];
	int n;

	repair_time = (repair_time + 1) % (NSHIP_SYSTEMS - 1); /* -1 to exclude hull */
	n = gs.player.damage[repair_time];
	if (n > 0) {
		n -= 15 + (xorshift(&xorshift_state) % 20);
		if (n <= 0) {
			n = 0;
			strcpy(msg, "CAPTAIN\n\nTHE ");
			strcat(msg, ship_system[repair_time]);
			strcat(msg, "\nHAS BEEN\nREPAIRED");
			alert_player("ENGINEERING", msg);
		}
		gs.player.damage[repair_time] = n;
	}
}

static void move_player(void)
{
	int dx, dy, b, nx, ny;
	int damage_factor;
	char decayed = 0;

	if (gs.player.docked) {
		gs.player.warp_factor = 0;
		replenish_supplies_and_repair_ship();
		return;
	}

	if ((xorshift(&xorshift_state) & 0x07) == 0) { /* 1 in 8 chance of dilithium crystal decaying. */
		int x = gs.player.dilithium_crystals - 1;
		if (x < 0)
			x = 0;
		gs.player.dilithium_crystals = x;
		decayed = 1;
	}

	/* Move player */
	if (gs.player.warp_factor > 0) {
		int warp_energy = (WARP_ENERGY_PER_SEC * gs.player.warp_factor) / WARP1;
		reduce_player_energy(warp_energy);
		
		b = (gs.player.heading * 128) / 360;
		if (b < 0)
			b += 128;
		if (b > 128)
			b -= 128;
		dx = (gs.player.warp_factor * cosine(b)) / 1024;
		dy = (-gs.player.warp_factor * sine(b)) / 1024;

		damage_factor = 100 - (100 * gs.player.damage[WARP_SYSTEM] / 255);

		nx = gs.player.x + damage_factor * dx / 100;
		ny = gs.player.y + damage_factor * dy / 100;

		if (coord_to_sector(gs.player.x) != coord_to_sector(nx) ||
			coord_to_sector(gs.player.y) != coord_to_sector(ny) ||
			coord_to_quadrant(gs.player.x) != coord_to_quadrant(nx) ||
			coord_to_quadrant(gs.player.y) != coord_to_quadrant(ny)) {
			gs.srs_needs_update = 1;
		}
		if (!player_collision_detection(&nx, &ny)) {
			gs.player.x = nx;
			gs.player.y = ny;
		}
	}

	conduct_repairs();

	if (100 * gs.player.damage[LIFE_SUPP_SYSTEM] / 255 > 70) {
		int n = gs.player.life_support_reserves - 1;
		if (n < 0)
			n = 0;
		gs.player.life_support_reserves = n;
		if (gs.player.life_support_reserves < 200 && (gs.player.life_support_reserves % 30) == 0) {
			alert_player("ENGINEERING", "CAPTAIN\n\nOUR LIFE\nSUPPORT SYSTEM\nIS BADLY\n"
							"DAMAGED AND\nRESERVES\nARE RUNNING\nLOW!\n");
			return;
		}
	}

	if (decayed) {
		if (gs.player.dilithium_crystals == 30 ||
			gs.player.dilithium_crystals == 20 ||
			gs.player.dilithium_crystals == 15) {
			alert_player("ENGINEERING", "CAPTAIN\n\nOUR SUPPLY OF\nDILITHIUM\nCRYSTALS IS\nDANGEROUSLY\nLOW\n");
			return;
		}
	}
}

static void fire_on_player(void)
{
	int i, damage, mitigation, shield_damage;

	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++) {
		damage = xorshift(&xorshift_state) % 75;
		if (damage < 50)
			continue;
		if (gs.player.shields_up) {
			mitigation = gs.player.shields * 85 / 255;
			shield_damage = 100 * gs.player.damage[SHIELD_SYSTEM] / 256;
			mitigation = (mitigation * shield_damage) / 100;
			damage = damage - mitigation;
			if (damage < 0)
				damage = 0;
			if (damage == 0)
				continue;
		}
		damage = gs.player.damage[i] + damage;
		if (damage > 255)
			damage = 255;
		gs.player.damage[i] = damage;
		gs.player.damage_flags |= (1 << i);
	}
}

static void move_enemy_ship(struct game_object *g)
{
	int angle, distance, x, y;

	if ((xorshift(&xorshift_state) % 1000) < 500) { /* 50 percent chance  */
		angle = (xorshift(&xorshift_state) % 128);
		distance = WARP1 * 3;
		x = g->x + cosine(angle) * (distance / 1024);
		y = g->y - sine(angle) * (distance / 1024);
		(void) keep_in_bounds(&x);
		(void) keep_in_bounds(&y);
		g->x = x;
		g->y = y;
		if (coord_to_sector(x) == coord_to_sector(gs.player.x) &&
			coord_to_sector(y) == coord_to_sector(gs.player.y)) {
			gs.srs_needs_update = 1;
		}
	}
	if (coord_to_sector(g->x) == coord_to_sector(gs.player.x) &&
		coord_to_sector(g->y) == coord_to_sector(gs.player.y)) {
		if ((xorshift(&xorshift_state) % 1000) < 500) { /* 50 percent chance  */
			fire_on_player();
		}
	}
}

static void move_enemies(void)
{
#define MOVEMENT_TRANCHES 5
	static int begin = 0;
	static int end = NTOTAL / MOVEMENT_TRANCHES;
	int i;

	/* Only move 1/10th of the objects each time. */
	for (i = begin; i < end; i++) {
		struct game_object *g = &gs.object[i];
		switch (g->type) {
		case ENEMY_SHIP:
			move_enemy_ship(g);
			break;
		default:
			continue;
		}
	}

	/* Set up to move the next set of 1/10 of the objects the next time we get called */
	begin = end;
	if (begin >= NTOTAL)
		begin = 0;
	end = begin + NTOTAL / MOVEMENT_TRANCHES;
	if (end > NTOTAL)
		end = NTOTAL;
}

static int player_has_won(void)
{
	int i;

	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case ENEMY_SHIP:
			return 0;
		default:
			break;
		}
	}
	gs.player.alive = 0;
	return 1;
}

static void move_objects(void)
{
	gs.player.damage_flags = 0;
	move_player();
	move_enemies();
}

static void draw_speed_gauge_ticks(void)
{
	int i;
	char num[3];

	FbColor(WHITE);
	FbVerticalLine(127, 5, 127, 105);
	num[2] = '\0';
	for (i = 0; i <= 10; i++) {
		FbHorizontalLine(120, 5 + i * 10, 126, 5 + i * 10);
		if ((i % 5) == 0) {
			FbMove(90, 5 + i * 10);
			if (i == 0)
				num[0] = '1';
			else
				num[0] = ' ';
			num[1] = '0' + ((10 - i) % 10);
			FbWriteLine(num);
		}
	}
}

static void draw_speed_gauge_marker(int color, int speed)
{
	int y1, y2, y3;

	y2 = 105 - ((100 * speed) >> 16);
	y1 = y2 - 5;
	y3 = y2 + 5;
	FbColor(color);
	FbVerticalLine(115, y1, 115, y3);
	FbLine(115, y1, 122, y2);
	FbLine(115, y3, 122, y2);
}

#define SPEED_GAUGE 0
#define PHASER_POWER_GAUGE 1 
static void draw_speed_gauge(int gauge_type, int color, int color2, int speed, int new_speed)
{
	draw_speed_gauge_ticks();
	draw_speed_gauge_marker(color, speed);
	draw_speed_gauge_marker(color2, new_speed);
	FbColor(GREEN);
	switch (gauge_type) {
	case SPEED_GAUGE:
		print_speed("CURR IMPULSE", "CURR WARP", 2, 30, speed);
		print_speed("NEW IMPULSE", "NEW WARP", 2, 60, new_speed);
		break;
	case PHASER_POWER_GAUGE:
		print_power(2, 30, speed);
		break;
	default:
		break;
	}
}

static void st_warp()
{
	if (gs.player.docked) {
		alert_player("WARP CONTROL", "CAPTAIN\n\n"
			"WE CANNOT\nENGAGE\nPROPULSION\nWHILE DOCKED\n"
			"WITH THE\nSTARBASE");
		return;
	}

	if (gs.player.standard_orbit) {
		alert_player("NAVIGATION", "CAPTAIN\nWE MUST LEAVE\nSTANDARD ORBIT\nBEFORE\nENGAGING THE\nWARP DRIVE");
		return;
	}

	screen_header("WARP");
	draw_speed_gauge(SPEED_GAUGE, GREEN, RED, gs.player.warp_factor, gs.player.new_warp_factor);

	if (gs.player.damage[WARP_SYSTEM] > 0) {
		FbMove(2, 110);
		FbWriteString("WARP SYS DAMAGED");
	}

	FbSwapBuffers();
	gs.last_screen = WARP_SCREEN;
	st_program_state = ST_WARP_INPUT;
}

static void st_phaser_power()
{
	screen_header("PHASER PWR");
	draw_speed_gauge(PHASER_POWER_GAUGE, GREEN, RED, gs.player.phaser_power, gs.player.new_phaser_power);

	FbMove(2, 110);
	print_numeric_item("AVAILABLE\nENERGY: ", gs.player.energy);

	FbSwapBuffers();
	gs.last_screen = PHASER_POWER_SCREEN;
	st_program_state = ST_PHASER_POWER_INPUT;
}

static void st_warp_input(void)
{
	int old = gs.player.new_warp_factor;

	int down_latches = button_down_latches();
	int rotary = button_get_rotation(0);
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		gs.player.warp_factor = gs.player.new_warp_factor;
		st_warp();
		st_program_state = ST_PROCESS_INPUT;
		return;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0) {
		gs.player.new_warp_factor += WARP1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0) {
		gs.player.new_warp_factor -= WARP1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		gs.player.new_warp_factor -= WARP1 / 10;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		gs.player.new_warp_factor += WARP1 / 10;
	}
	if (gs.player.new_warp_factor < 0)
		gs.player.new_warp_factor = 0;
	if (gs.player.new_warp_factor > WARP10)
		gs.player.new_warp_factor = WARP10;

	if (old != gs.player.new_warp_factor)
		st_program_state = ST_WARP;
}

static void st_shield_control(void)
{
	clear_menu();
	strcpy(menu.title, "SHIELD CONTROL");
	strcpy(menu.title2, "SHIELDS: ");
	if (gs.player.shields_up)
		strcat(menu.title2, "UP");
	else
		strcat(menu.title2, "DOWN");
	strcpy(menu.title3, "ENERGY: ");
	strcatnum(menu.title3, (MAX_SHIELD_ENERGY * gs.player.shields) / 255);
	dynmenu_add_item(&menu, "SHIELDS UP", ST_SHIELDS_UP, 0);
	dynmenu_add_item(&menu, "SHIELDS DOWN", ST_SHIELDS_DOWN, 0);
	dynmenu_add_item(&menu, "ENERGY XFER", ST_SHIELD_ENERGY, 0);
	dynmenu_add_item(&menu, "CANCEL ORDER", ST_CAPTAIN_MENU, 0);
	menu.menu_active = 1;
	st_program_state = ST_DRAW_MENU;
}

static void st_shields_updown(int x)
{
	char msg[50];
	int energy, damage;

	gs.player.shields_up = x;
	energy = (2500 * gs.player.shields) / 255;
	damage = (100 * gs.player.damage[SHIELD_SYSTEM]) / 255;
	if (x) {
		if (gs.player.energy < SHIELDS_UP_ENERGY) {
			alert_player("SHIELDS CONTROL", "CAPTAIN\n\nTHERE IS\nINSUFFICIENT\n"
							"ENERGY TO\nRAISE SHIELDS");
			return;
		}
		reduce_player_energy(SHIELDS_UP_ENERGY);
		strcpy(msg, "SHIELDS UP\n");
	} else {
		strcpy(msg, "SHIELDS DOWN\n");
	}
	strcat(msg, "ENERGY: ");
	strcatnum(msg, energy);
	strcat(msg, "\nDAMAGE: ");
	strcatnum(msg, damage);
	strcat(msg, "%");
	alert_player("SHIELD CONTROL", msg);
}

static void st_shield_energy(void)
{
	int shield_energy, min, max, max_ship;

	shield_energy = (MAX_SHIELD_ENERGY * gs.player.shields) / 255;
	min = -shield_energy;
	max = MAX_SHIELD_ENERGY - shield_energy;
	max_ship = INITIAL_ENERGY - gs.player.energy;
	if (-max_ship > min)
		min = -max_ship;

	if (max == 0 && min == 0) {
		alert_player("SHIELD CONTROL", "CAPTAIN\n\nNO ENERGY XFER\nIS POSSIBLE\n\n"
				"SHIP ENERGY\nAND SHIELD\nENERGY BOTH\nAT MAXIMUM\nALREADY");
		return;
	}

	FbClear();
	FbMove(2, 2);
	FbColor(WHITE);
	FbWriteString("SHIELD ENERGY\n");
	FbColor(GREEN);
	print_numeric_item("SHLD ENRG: ", shield_energy);
	print_numeric_item("SHIP ENRG: ", gs.player.energy);
	print_numeric_item("MIN XFER: ", min);
	print_numeric_item("MAX XFER: ", max);
	FbColor(WHITE);
	FbWriteString("\n");
	print_numeric_item("XFER?  ", gs.player.new_shield_xfer);
	FbColor(GREEN);
	FbWriteString("\nL/R - 100 UNITS\n");
	FbWriteString("U/D - 500 UNITS\n");
	FbColor(WHITE);
	if (gs.player.new_shield_xfer < 0)
		FbWriteString("XFER TO SHIP\n");
	else if (gs.player.new_shield_xfer > 0)
		FbWriteString("XFER TO SHIELDS\n");
	else
		FbWriteString("0 ENERGY XFER\n");
	FbColor(GREEN);
	FbWriteString("2% ENERGY LOSS\nINCURRED ON\nXFER");
	FbSwapBuffers();
	st_program_state = ST_SHIELD_ENERGY_INPUT;
}

static void st_shield_energy_input()
{
	int old = gs.player.new_shield_xfer;
	int shield_energy, min, max, max_ship;
	shield_energy = (MAX_SHIELD_ENERGY * gs.player.shields) / 255;
	min = -shield_energy;
	max = MAX_SHIELD_ENERGY - shield_energy;
	max_ship = INITIAL_ENERGY - gs.player.energy;
	if (-max_ship > min)
		min = -max_ship;

	int down_latches = button_down_latches();
	int rotary = button_get_rotation(0);
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		gs.player.shield_xfer = gs.player.new_shield_xfer;
		st_program_state = ST_SHIELD_EXEC_ENERGY_XFER;
		return;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0) {
		gs.player.new_shield_xfer += 500;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0) {
		gs.player.new_shield_xfer -= 500;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		gs.player.new_shield_xfer -= 100;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		gs.player.new_shield_xfer += 100;
	}
	if (gs.player.new_shield_xfer < min)
		gs.player.new_shield_xfer = min;
	if (gs.player.new_shield_xfer > max)
		gs.player.new_shield_xfer = max;

	if (old != gs.player.new_shield_xfer)
		st_program_state = shield_energy;
	st_program_state = ST_SHIELD_ENERGY;
}

static void st_shield_exec_energy_xfer(void)
{
	int from_shields, two_percent;

	if (gs.player.shield_xfer == 0) {
		alert_player("SHIELD CONTROL", "CAPTAIN\n\nZERO ENERGY\nTRANSFERED");
		return;
	}
	if (gs.player.shield_xfer > 0) { /* xfer > 0, ship to shields xfer */
		int to_shields;

		two_percent = (2 * gs.player.shield_xfer) / 100;

		gs.player.energy -= gs.player.shield_xfer;
		to_shields = (255 * (gs.player.shield_xfer - two_percent)) / MAX_SHIELD_ENERGY;
		to_shields += gs.player.shields;
		if (to_shields > 255)
			to_shields = 255;
		gs.player.shields = to_shields;
		alert_player("SHIELD CONTROL", "CAPTAIN\n\nENERGY XFER\nFROM SHIP\nTO SHIELDS\nCOMPLETE");
		return;
	}
	/* else xfer < 0, shields to ship xfer */

	two_percent = (2 * -gs.player.shield_xfer) / 100;
	gs.player.energy += (-gs.player.shield_xfer) - two_percent;
	from_shields = (255 * -gs.player.shield_xfer) / MAX_SHIELD_ENERGY;
	from_shields = gs.player.shields - from_shields;
	if (from_shields < 0)
		from_shields = 0;
	gs.player.shields = from_shields;
	alert_player("SHIELD CONTROL", "CAPTAIN\n\nENERGY XFER\nFROM SHIELDS\nTO SHIP\nCOMPLETE");
}

static void st_phaser_power_input(void)
{
	int down_latches = button_down_latches();
	int rotary = button_get_rotation(0);
	int old = gs.player.new_phaser_power;

	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches)) {
		gs.player.phaser_power = gs.player.new_phaser_power;
		st_phaser_power();
		st_program_state = ST_FIRE_PHASER;
		return;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0) {
		gs.player.new_phaser_power += WARP1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0) {
		gs.player.new_phaser_power -= WARP1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		gs.player.new_phaser_power -= WARP1 / 10;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		gs.player.new_phaser_power += WARP1 / 10;
	}
	if (gs.player.new_phaser_power < 0)
		gs.player.new_phaser_power = 0;
	if (gs.player.new_phaser_power > WARP10)
		gs.player.new_phaser_power = WARP10;

	if (old != gs.player.new_phaser_power)
		st_program_state = ST_PHASER_POWER;
}

static int check_weapon_collision(char *weapon_name, int i, int x, int y)
{
	int tx, ty; 
	char msg[40];

	tx = gs.object[i].x;
	ty = gs.object[i].y;

	/* Can't use pythagorean theorem because square of distance across 1 quadrant
	 * will overflow an int.
	 */
	if (coord_to_sector(tx) != coord_to_sector(x))
		return 0;
	if (coord_to_sector(ty) != coord_to_sector(y))
		return 0;
	if (coord_to_quadrant(tx) != coord_to_quadrant(x))
		return 0;
	if (coord_to_quadrant(ty) != coord_to_quadrant(y))
		return 0;
	strcpy(msg, weapon_name);
	strcat(msg, " HITS\n");
	strcat(msg, object_type_name(gs.object[i].type));
	strcat(msg, "!");
	alert_player(weapon_name, msg);
	
	return 1;
}

static void do_weapon_damage(int target, int power)
{
	int new_hp, scaled_power, damage;

	if (gs.object[target].type != ENEMY_SHIP)
		return;

	scaled_power = (power >> 11) & 0x0ff;
	damage = 256 + (xorshift(&xorshift_state) & 0x0ff) / 2;
	damage = (damage * scaled_power) / 256;
	new_hp = gs.object[target].tsd.ship.hitpoints;
	new_hp -= damage;
	if (new_hp <= 0) {
		adjust_score_on_kill(target);
		delete_object(target);
		alert_player("WEAPONS", "ENEMY SHIP\nDESTROYED!");
		return;
	}
	gs.object[target].tsd.ship.hitpoints = new_hp;
	return;
}

static void st_move_weapon(void)
{
	int j;
	struct weapon_t *w = &gs.weapon;

	if (w->i <= 0) {
		alert_player(w->name, "MISSED!");
		memset(w, 0, sizeof(*w));
		return;
	}

	for (j = 0; j < NTOTAL; j++) {
		switch (gs.object[j].type) {
		case ENEMY_SHIP:
			if (check_weapon_collision(w->name, j, w->x, w->y)) {
				do_weapon_damage(j, w->damage_factor * w->weapon_power / 100);
				gs.weapon.i = 0; /* kill this weapon */
				return;
			}
			break;
		default:
			continue;
		}
	}
	w->x += w->dx;
	w->y += w->dy;
	w->i--;
}

static void st_fire_weapon(char *weapon_name, int weapon_power)
{
	int a;
	int damage_factor;

	damage_factor = 100;
	if (weapon_name[0] == 'P' && weapon_name[1] == 'H') { /* PHASER? */
		int energy_units;
		/* Check if we have enough power */
		energy_units = PHASER_ENERGY * 10 * weapon_power / (1 << 16);
		if (gs.player.energy < energy_units) {
			alert_player("WEAPONS", "CAPTAIN\n\nWE DON'T\nHAVE ENOUGH\nENERGY TO\n"
						"FIRE PHASERS\nAT THAT\nLEVEL");
			return;
		}
		damage_factor = 100 - (100 * gs.player.damage[PHASER_SYSTEM] / 255);
		if (damage_factor < 10) {
			alert_player("WEAPONS", "CAPTAIN\n\nTHE PHASER\nSYSTEM IS SO\nDAMAGED IT\nCANNOT BE\nUSED");
			return;
		}
		reduce_player_energy(energy_units);
	} else { /* must be a torpedo */
		reduce_player_energy(TORPEDO_ENERGY);
	}

	a = (gs.player.weapons_aim * 128) / 360;
	gs.weapon.dx = (cosine(a) * WEAPON_SPEED) / 1024;
	gs.weapon.dy = (-sine(a) * WEAPON_SPEED) / 1024;
	gs.weapon.i = WEAPON_LIFETIME;
	gs.weapon.x = gs.player.x;
	gs.weapon.y = gs.player.y;
	gs.weapon.damage_factor = damage_factor;
	gs.weapon.weapon_power = weapon_power;
	gs.weapon.name = weapon_name;

	st_program_state = ST_MOVE_WEAPON;
	return;
}

static void st_player_died(char *msg, char *first_menu_item, enum st_program_state_t state)
{
	clear_menu();
	strcpy(menu.title, "GAME OVER");
	strcpy(menu.title2, "PLAY AGAIN?");
	dynmenu_add_item(&menu, first_menu_item, state, 0);
	dynmenu_add_item(&menu, "PLAY AGAIN", ST_NEW_GAME, 0);
	dynmenu_add_item(&menu, "QUIT", ST_EXIT, 0);
	menu.menu_active = 1;
	dynmenu_draw(&menu);
	FbMove(2, 21);
	FbColor(CYAN);
	FbWriteString(msg);
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void st_hull_destroyed(void)
{
	gs.player.alive = 0;
	st_player_died("HULL BREACHED\nYOU HAVE DIED\n", "OH NO!", st_program_state);
}

static void st_life_support_failed(void)
{
	gs.player.alive = 0;
	st_player_died("LIFE SUPPORT\nFAILED. YOU\nHAVE DIED OF\nASPHYXIATION", "OH NO!", st_program_state);
}

static void st_player_won(void)
{
	char msg[80];

	strcpy(msg, "YOU DESTROYED\nALL THE ENEMY\nSHIPS. YOUR\nFINAL SCORE\nWAS ");
	strcatnum(msg, gs.score);
	st_player_died(msg, "CONGRATS!", st_program_state);
}

static void st_alert(void)
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		st_program_state = ST_CAPTAIN_MENU;
}

static void st_photon_torpedoes(void)
{
	if (gs.player.energy < TORPEDO_ENERGY) {
		alert_player("WEAPONS", "CAPTAIN\n\nWE DON'T HAVE\nENOUGH\n"
					"ENERGY TO\nLAUNCH A\nTORPEDO!");
		return;
	}
	if (100 * gs.player.damage[TORPEDO_SYSTEM] / 255 >= 90) {
		alert_player("WEAPONS", "CAPTAIN\n\nTHE TORPEDO\nSYSTEM IS TOO\nDAMAGED TO BE\nUSED");
		return;
	}

	if (gs.player.torpedoes > 0) {
		st_fire_weapon(torpedo_name, TORPEDO_POWER);
		gs.player.torpedoes--;
	} else {
		alert_player("WEAPONS", "NO PHOTON\nTORPEDOES\nREMAIN, SIR!");
	}
}

static void st_phaser_beams(void)
{
	st_program_state = ST_PHASER_POWER;
}

static int time_to_move_objects(void) /* Returns true once per second */
{
	static int ticks = 0;

	/* Don't move stuff while the player is trying to read an alert */
	if (st_program_state == ST_ALERT)
		return 0;

	/* Don't move stuff while the player is trying to read a damage alert */
	if (gs.last_screen == REPORT_DAMAGE_SCREEN && st_program_state == ST_PROCESS_INPUT)
		return 0;


	int new_ticks = get_time();
	if (new_ticks != ticks) {
		ticks = new_ticks;
		return 1;
	}
	return 0;
}

static void report_damage(void)
{
	int i;

	FbClear();
	FbColor(WHITE);
	FbMove(2, 2);
	FbWriteString("CAPTAIN\n\nWE HAVE BEEN\nHIT BY ENEMY\nFIRE\n\nDAMAGE TO:\n");
	FbColor(CYAN);
	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++) {
		if (gs.player.damage_flags & (1 << i)) {
			FbWriteString((char *) ship_system[i]);
			FbMoveX(2);
			FbMoveRelative(0, 8);
		}
	}
	FbSwapBuffers();
	gs.last_screen = REPORT_DAMAGE_SCREEN;
	gs.player.damage_flags = 0;
	st_program_state = ST_PROCESS_INPUT;
}

static int time_has_run_out(void)
{
	gs.stardate++; /* advance game time */
	if (gs.stardate >= gs.enddate) {
		alert_player("STARFLEET", "YOU HAVE RUN\nOUT OF TIME\nWE WILL GIVE\n"
						"YOU AN\nADDITIONAL\n10 DAYS.");
		gs.enddate += 10 * TICKS_PER_DAY;
		return 1;
	}
	return 0;
}

int spacetripper_cb(void)
{
	switch (st_program_state) {
	case ST_GAME_INIT:
		st_game_init();
		break;
	case ST_NEW_GAME:
		st_new_game();
		break;
	case ST_CAPTAIN_MENU:
		st_captain_menu();
		break;
	case ST_EXIT:
		st_program_state = ST_GAME_INIT;
		returnToMenus();
		break;
	case ST_DRAW_MENU:
		st_draw_menu();
		break;
	case ST_RENDER_SCREEN:
		st_render_screen();
		break;
	case ST_PROCESS_INPUT:
		st_process_input();
		break;
	case ST_LRS:
		st_lrs();
		break;
	case ST_STAR_CHART:
		st_star_chart();
		break;
	case ST_SRS:
		st_srs();
		break;
	case ST_SRS_LEGEND:
		st_srs_legend();
		break;
	case ST_SET_COURSE:
		st_set_course();
		break;
	case ST_AIM_WEAPONS:
		if (gs.player.docked) {
			alert_player("WEAPONS", "CAPTAIN\n\nWE CANNOT FIRE\nWHILE DOCKED AT\nTHE STARBASE");
			break;
		}
		gs.angle_chooser.finished = 0;
		st_choose_angle("AIM WEAPONS: ", "CUR AIM", "SET AIM: ",
				&gs.player.weapons_aim, &gs.player.new_weapons_aim, AIMING_SCREEN,
				show_energy_and_torps);
		break;
	case ST_CHOOSE_ANGLE_INPUT:
		st_choose_angle_input();
		break;
	case ST_DRAW_ANGLE_CHOOSER:
		st_draw_angle_chooser();
		break;
	case ST_CHOOSE_WEAPONS:
		st_choose_weapons();
		break;
	case ST_WARP:
		st_warp();
		break;
	case ST_WARP_INPUT:
		st_warp_input();
		break;
	case ST_SHIELD_CONTROL:
		st_shield_control();
		break;
	case ST_SHIELDS_UP:
		st_shields_updown(1);
		break;
	case ST_SHIELDS_DOWN:
		st_shields_updown(0);
		break;
	case ST_SHIELD_ENERGY:
		st_shield_energy();
		break;
	case ST_SHIELD_ENERGY_INPUT:
		st_shield_energy_input();
		break;
	case ST_SHIELD_EXEC_ENERGY_XFER:
		st_shield_exec_energy_xfer();
		break;
	case ST_PLANETS:
		st_planets();
		break;
	case ST_DOCK:
		st_dock();
		break;
	case ST_TRANSPORTER:
		st_transporter();
		break;
	case ST_MINE_DILITHIUM:
		st_mine_dilithium();
		break;
	case ST_LOAD_DILITHIUM:
		st_load_dilithium();
		break;
	case ST_SELF_DESTRUCT_CONFIRM:
		st_self_destruct_confirm();
		break;
	case ST_SELF_DESTRUCT:
		st_self_destruct();
		break;
	case ST_STANDARD_ORBIT:
		st_standard_orbit();
		break;
	case ST_SENSORS:
		st_sensors();
		break;
	case ST_DAMAGE_REPORT:
		st_damage_report();
		break;
	case ST_STATUS_REPORT:
		st_status_report();
		break;
	case ST_ALERT:
		st_alert();
		break;
	case ST_PHOTON_TORPEDOES:
		st_photon_torpedoes();
		break;
	case ST_PHASER_BEAMS:
		st_phaser_beams();
		break;
	case ST_PHASER_POWER:
		st_phaser_power();
		break;
	case ST_PHASER_POWER_INPUT:
		st_phaser_power_input();
		break;
	case ST_FIRE_PHASER:
		st_fire_weapon(phaser_name, gs.player.phaser_power);
		break;
	case ST_MOVE_WEAPON:
		st_move_weapon();
		break;
	case ST_HULL_DESTROYED:
		st_hull_destroyed();
		break;
	case ST_LIFE_SUPPORT_FAILED:
		st_life_support_failed();
		break;
	case ST_PLAYER_WON:
		st_player_won();
		break;
	default:
		st_program_state = ST_CAPTAIN_MENU;
		break;
	}

	if (!gs.player.alive)
		return 0;

	if (time_to_move_objects()) {
		if (time_has_run_out())
			return 1;
		move_objects();
		if (gs.last_screen == SRS_SCREEN && gs.srs_needs_update)
			st_program_state = ST_SRS;
	}

	if (gs.player.damage_flags)
		report_damage();

	if (gs.player.damage[HULL_SYSTEM] == 255)
		st_program_state = ST_HULL_DESTROYED;
	if (100 * gs.player.damage[LIFE_SUPP_SYSTEM] > 70 && gs.player.life_support_reserves == 0)
		st_program_state = ST_LIFE_SUPPORT_FAILED;
	if (player_has_won())
		st_program_state = ST_PLAYER_WON;
	return 0;
}

