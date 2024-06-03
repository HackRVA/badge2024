/*
   simple menu system
   Author: Paul Bruggeman
   paul@Killercats.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TARGET_SIMULATOR
#include <unistd.h>
#include <signal.h> /* for raise() */
#endif
#include "menu.h"
#include "settings.h"
#include "colors.h"
#include "ir.h"
#include "assetList.h"
#include "button.h"
#include "framebuffer.h"
#include "display.h"
#include "audio.h"
#include "led_pwm.h"
#include "music.h"
#include "menu_icon.h"
#include "stacktrace.h"
#include "key_value_storage.h"

// Apps
#include "about_badge.h"
#include "new_badge_monsters/new_badge_monsters.h"
#include "battlezone.h"
#include "game_of_life.h"
#include "hacking_simulator.h"
#include "lunarlander.h"
// #include "pong.h"
#include "qc.h"
#include "smashout.h"
#include "username.h"
#include "slot_machine.h"
// #include "gulag.h"
#include "asteroids.h"
// #include "etch-a-sketch.h"
// #include "magic-8-ball.h"
#include "rvasec_splash.h"
#include "test-screensavers.h"
// #include "tank-vs-tank.h"
#include "clue.h"
#include "moon-patrol.h"
// #include "badgey.h"
#include "badge-app-template.h"
#include "aagunner.h"
#include "rover_adventure.h"

/* BUILD_IMAGE_TEST_PROGRAM is defined (or not) in top level CMakelists.txt */
#ifdef BUILD_IMAGE_TEST_PROGRAM
#include "image-test.h"
#endif

#define MAIN_MENU_BKG_COLOR GREY2


extern const struct menu_t schedule_m[]; /* defined in core/schedule.c */

static const struct menu_t games_m[] = {
	// {"Sample App", VERT_ITEM, FUNCTION, { .func = myprogram_cb }, NULL },
	{"Rover Adventure", VERT_ITEM|DEFAULT_ITEM, FUNCTION, { .func = rover_adventure_cb }, &bba_icon },
	{"Badge Monsters",VERT_ITEM, FUNCTION, { .func = badge_monsters_cb }, &badge_monsters_icon, },
	{"Moon Patrol", VERT_ITEM, FUNCTION, { .func = moonpatrol_cb }, &moonpatrol_icon, },
	{"AA Gunner", VERT_ITEM, FUNCTION, { .func = aagunner_cb }, &aagunner_icon },
	{"Clue", VERT_ITEM, FUNCTION, { .func = clue_cb }, &clue_icon },
	// {"Badgey", VERT_ITEM, FUNCTION, { .func = badgey_cb }, &bba_icon },
	{"Asteroids", VERT_ITEM, FUNCTION, { .func = asteroids_cb }, &asteroids_icon, },
	{"Lunar Rescue",  VERT_ITEM, FUNCTION, { .func = lunarlander_cb}, &lunar_rescue_icon, },
	{"Battlezone", VERT_ITEM, FUNCTION, { .func = battlezone_cb }, &battlezone_icon, },
	{"Slot Machine", VERT_ITEM, FUNCTION, { .func = slot_machine_cb }, &slotmachine_icon, },
	{"Smashout",      VERT_ITEM, FUNCTION, { .func = smashout_cb }, &breakout_icon, },
	{"Hacking Sim",   VERT_ITEM, FUNCTION, { .func = hacking_simulator_cb }, &hacker_sim_icon, },
	{"Game of Life", VERT_ITEM, FUNCTION, { .func = game_of_life_cb }, &game_of_life_icon, },
#ifdef BUILD_IMAGE_TEST_PROGRAM
	{"Image Test", VERT_ITEM, FUNCTION, { .func = image_test_cb }, NULL },
#endif
	// {"Etch-a-Sketch", VERT_ITEM, FUNCTION, { .func = etch_a_sketch_cb }, NULL, },
	// {"Magic-8-Ball",     VERT_ITEM, FUNCTION, { .func = magic_8_ball_cb }, NULL, },
	// {"Goodbye Gulag", VERT_ITEM, FUNCTION, { .func = gulag_cb }, NULL, },
	// {"Pong", VERT_ITEM, FUNCTION, { .func = pong_cb }, NULL, },
	// {"Tank vs Tank", VERT_ITEM, FUNCTION, { .func = tank_vs_tank_cb }, NULL, },
	{"Back",         VERT_ITEM|LAST_ITEM, BACK, { NULL }, NULL, },
};

static const struct menu_t settings_m[] = {
   {"Backlight", VERT_ITEM, MENU, { .menu = backlight_m }, &backlight_icon, },
   {"LED", VERT_ITEM, MENU, { .menu = LEDlight_m }, &led_icon, },
   {"Audio", VERT_ITEM|DEFAULT_ITEM, MENU, { .menu = buzzer_m }, &audio_icon, },
   {"Invert Display", VERT_ITEM, MENU, { .menu = rotate_m, }, &invert_display_icon, },
   {"User Name", VERT_ITEM, FUNCTION, { .func = username_cb }, &username_icon, },
   {"Screensaver", VERT_ITEM, MENU, { .menu = screen_lock_m }, &screensaver_icon, },
   {"ID", VERT_ITEM, MENU, { .menu = myBadgeid_m }, &id_icon, },
   {"QC",  VERT_ITEM, FUNCTION, { .func = QC_cb }, &qc_icon, },
   {"Clear NVRAM", VERT_ITEM, FUNCTION, { .func = clear_nvram_cb }, &clear_nvram_icon, },
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, {NULL}, NULL, },
};

const struct menu_t main_m[] = {
   {"Games",       VERT_ITEM|DEFAULT_ITEM, MENU, { .menu = games_m }, &games_icon, },
   {"Schedule",    VERT_ITEM, MENU, { .menu = schedule_m }, &schedule_icon, },
   {"Settings",    VERT_ITEM, MENU, { .menu = settings_m }, &settings_icon, },
   // {"Test SS",	VERT_ITEM, FUNCTION, { .func = test_screensavers_cb }, NULL, },
   {"About Badge",    VERT_ITEM|LAST_ITEM, FUNCTION, { .func = about_badge_cb }, &about_icon, },
};

