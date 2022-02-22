#include "menu.h"

#ifndef NULL
#define NULL 0
#endif

const struct menu_t breakfast_m[] = {
//   {"yummy", VERT_ITEM|DEFAULT_ITEM, FUNCTION, {(struct menu_t *)yummy_unlock_1}}, /* can init union either with or without {} */
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t day2_p2_m[] = {
   {"11:50 Lunch", VERT_ITEM, TEXT, {NULL}},
   {" 1:00 Bake Sec", VERT_ITEM, TEXT, {NULL}},
   {" 1:50 -break-", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 5M+ Vulns", VERT_ITEM, TEXT, {NULL}},
   {" 2:50 -break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 3 Worlds", VERT_ITEM, TEXT, {NULL}},
   {" 3:50 Closing", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t day2_p1_m[] = {
   {" 8:00 Breakfast", VERT_ITEM, MENU, {breakfast_m}}, // breakfast easter egg
   {" 8:00 Registrat", VERT_ITEM, TEXT, {NULL}},
   {" 8:50 Welcome", VERT_ITEM, TEXT, {NULL}},
   {" 9:00 Mentors", VERT_ITEM, TEXT, {NULL}},
   {" 9:50 -break-", VERT_ITEM, TEXT, {NULL}},
   {"10:00 CMMC", VERT_ITEM, TEXT, {NULL}},
   {"10:00 CTF Comp", VERT_ITEM, TEXT, {NULL}},
   {"11:00 Quackery", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t day1_p3_m[] = {
   {" 2:00 Rnsmware", VERT_ITEM, TEXT, {NULL}},
   {" 2:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 Infra code", VERT_ITEM, TEXT, {NULL}},
   {" 3:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 4:00 Innerloop", VERT_ITEM, TEXT, {NULL}},
   {" 4:50 Closing", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t day1_p2_m[] = {
   {"11:00 Purple", VERT_ITEM, TEXT, {NULL}},
   {"11:50 Lunch", VERT_ITEM, TEXT, {NULL}},
   {" 1:00 0 Trust", VERT_ITEM, TEXT, {NULL}},
   {" 1:00 CTF Prep", VERT_ITEM, TEXT, {NULL}},
   {" 1:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t day1_p1_m[] = {
   {" 8:00 Breakfast", VERT_ITEM, MENU, {breakfast_m}},
   {" 8:00 Registrat", VERT_ITEM, TEXT, {NULL}},
   {" 9:00 Welcome", VERT_ITEM, TEXT, {NULL}},
   {" 9:10 Keynote", VERT_ITEM, TEXT, {NULL}},
   {"10:10 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"10:30 CTF Intro", VERT_ITEM, TEXT, {NULL}},
   {"10:40 Badge", VERT_ITEM, TEXT, {NULL}},
   {"10:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t schedule_m[] = {
   {"Thursday", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}},
   {"  8-10:50", VERT_ITEM|DEFAULT_ITEM, MENU, {day1_p1_m}},
   {"  11-1:50", VERT_ITEM, MENU, {day1_p2_m}},
   {"  2-4:50", VERT_ITEM, MENU, {day1_p3_m}},
   {"  5:30 Aft Prty", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}},
   {"Friday", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}},
   {"  8-11:00", VERT_ITEM, MENU, {day2_p1_m}},
   {"  11:50-Close", VERT_ITEM, MENU, {day2_p2_m}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

