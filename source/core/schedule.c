#include "menu.h"

#ifndef NULL
#define NULL 0
#endif

const struct menu_t day1_breakfast_m[] = {
   {"Sausage&Egg on", VERT_ITEM, TEXT, {NULL}},
   {"  Buttmlk. Bisc", VERT_ITEM, TEXT, {NULL}},
   {"EngMuf w/ EggW,", VERT_ITEM, TEXT, {NULL}},
   {"  Spin., Mush.", VERT_ITEM, TEXT, {NULL}},
   {"Fruit Cup", VERT_ITEM, TEXT, {NULL}},
   {"Ch./Choc./Appl.", VERT_ITEM, TEXT, {NULL}},
   {"  Danish", VERT_ITEM, TEXT, {NULL}},
   {"Donut Holes", VERT_ITEM, TEXT, {NULL}},
   {"Appl./O. Juic", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t day2_breakfast_m[] = {
   {"Egg, Potato, &", VERT_ITEM, TEXT, {NULL}},
   {"  Bacon Burrito", VERT_ITEM, TEXT, {NULL}},
   {"GF EggW, Spin.", VERT_ITEM, TEXT, {NULL}},
   {"  Pep., On. Wrp", VERT_ITEM, TEXT, {NULL}},
   {"Fruit Cup", VERT_ITEM, TEXT, {NULL}},
   {"Coffee Cakes", VERT_ITEM, TEXT, {NULL}},
   {"Muffins", VERT_ITEM, TEXT, {NULL}},
   {"Appl./O. Juic", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t day1_lunch_m[] = {
   {"Box w/ roll,", VERT_ITEM, TEXT, {NULL}},
   {"  slaw, chip,", VERT_ITEM, TEXT, {NULL}},
   {"  brownie.", VERT_ITEM, TEXT, {NULL}},
   {"Choice of:", VERT_ITEM, TEXT, {NULL}},
   {"  BBQ Chicken", VERT_ITEM, TEXT, {NULL}},
   {"  Port. Mush.", VERT_ITEM, TEXT, {NULL}},
   {"Beverage", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t day2_lunch_m[] = {
   {"Box w/ Fruit,", VERT_ITEM, TEXT, {NULL}},
   {"  cup, potato,", VERT_ITEM, TEXT, {NULL}},
   {"  salad, lemon", VERT_ITEM, TEXT, {NULL}},
   {"  bar.", VERT_ITEM, TEXT, {NULL}},
   {"Choice of:", VERT_ITEM, TEXT, {NULL}},
   {"  Chicken Salad", VERT_ITEM, TEXT, {NULL}},
   {"  Club Sandwich", VERT_ITEM, TEXT, {NULL}},
   {"  V&GF Grdn Wrp", VERT_ITEM, TEXT, {NULL}},
   {"Beverage", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t day2_p2_m[] = {
   {"11:50 Lunch", VERT_ITEM, MENU, {day2_lunch_m}},
   {" 1:00 Threat IQ", VERT_ITEM, TEXT, {NULL}},
   {" 1:50 -break-", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 To Err", VERT_ITEM, TEXT, {NULL}},
   {" 2:50 -break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 Malware", VERT_ITEM, TEXT, {NULL}},
   {" 3:50 Closing", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

const struct menu_t day2_p1_m[] = {
   {" 8:00 Breakfast", VERT_ITEM, MENU, {day2_breakfast_m}},
   {" 8:00 Registrat", VERT_ITEM, TEXT, {NULL}},
   {" 8:50 Welcome", VERT_ITEM, TEXT, {NULL}},
   {" 9:00 FBI", VERT_ITEM, TEXT, {NULL}},
   {" 9:50 -break-", VERT_ITEM, TEXT, {NULL}},
   {"10:00 DigiForen", VERT_ITEM, TEXT, {NULL}},
   {"10:00 CTF Comp", VERT_ITEM, TEXT, {NULL}},
   {"11:00 Bass/Base", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t day1_p3_m[] = {
   {" 2:00 1stAppSec", VERT_ITEM, TEXT, {NULL}},
   {" 2:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 Talk the", VERT_ITEM, TEXT, {NULL}},
   {" 3:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 4:00 Warning:", VERT_ITEM, TEXT, {NULL}},
   {" 4:50 Closing", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t day1_p2_m[] = {
   {"11:00 Honeypot", VERT_ITEM, TEXT, {NULL}},
   {"11:50 Lunch", VERT_ITEM, MENU, {day1_lunch_m}},
   {" 1:00 Slippery", VERT_ITEM, TEXT, {NULL}},
   {" 1:00 CTF Prep", VERT_ITEM, TEXT, {NULL}},
   {" 1:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t day1_p1_m[] = {
   {" 8:00 Breakfast", VERT_ITEM, MENU, {day1_breakfast_m}},
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
   {"  5:00 Aft Prty", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}},
   {"Friday", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}},
   {"  8-11:00", VERT_ITEM, MENU, {day2_p1_m}},
   {"  11:50-Close", VERT_ITEM, MENU, {day2_p2_m}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

