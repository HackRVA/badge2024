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

const struct menu_t day2_p1_m[] = {
   {"Wednesday", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}},
   {" 7:59 Registrat", VERT_ITEM, TEXT, {NULL}},
   {" 8:00 Breakfast", VERT_ITEM, MENU, {day2_breakfast_m}},
   {" 8:50 Welcome", VERT_ITEM, TEXT, {NULL}},
   {" 9:00 Keynote", VERT_ITEM, TEXT, {NULL}},
   {"10:00 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"10:00 CTF Comp", VERT_ITEM, TEXT, {NULL}},
   {"10:00 Badge", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Badge Repair\n\n"
		"Come learn about\n"
		"your badge, get\n"
		"it fixed if\n"
		"there are any\n"
		"issues and talk\n"
		"to HackRVA!\n",
	},
   },
   {"10:00 Lock Pick", VERT_ITEM, TEXT, {NULL}},
   {"10:30 NSA", VERT_ITEM, TEXT, {NULL}},
   {"10:30 CISO", VERT_ITEM, TEXT, {NULL}},
   {"10:30 Heap Exp", VERT_ITEM, TEXT, {NULL}},
   {"11:20 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"11:30 NIST/CMMC", VERT_ITEM, TEXT, {NULL}},
   {"11:30 Lead Creed", VERT_ITEM, TEXT, {NULL}},
   {"11:30 Intruder", VERT_ITEM, TEXT, {NULL}},
   {"12:20 Lunch", VERT_ITEM, MENU, {day2_lunch_m}},
   {" 1:00 Quantum", VERT_ITEM, TEXT, {NULL}},
   {" 1:00 Cmnwealth", VERT_ITEM, TEXT, {NULL}},
   {" 1:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 ChatGPT", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 Ransomware", VERT_ITEM, TEXT, {NULL}},
   {" 2:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:10 Insiders", VERT_ITEM, TEXT, {NULL}},
   {" 4:00 Closing", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

#if 0
const struct menu_t day1_p3_m[] = {
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t day1_p2_m[] = {
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};
#endif

const struct menu_t day1_p1_m[] = {
   {"Tuesday", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}},
   {" 7:59 Registrat", VERT_ITEM, TEXT, {NULL}},
   {" 8:00 Breakfast", VERT_ITEM, MENU, {day1_breakfast_m}},
   {" 9:00 Welcome", VERT_ITEM, TEXT, {NULL}},
   {" 9:30 Keynote", VERT_ITEM, TEXT, {NULL}},
   {"10:00 Badge Tr", VERT_ITEM, TEXT, {NULL}},
   {"10:00 Lock Pick", VERT_ITEM, TEXT, {NULL}},
   {"10:30 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"11:00 SW Liablty", VERT_ITEM, TEXT, {NULL}},
   {"11:50 Lunch", VERT_ITEM, MENU, {day1_lunch_m}},
   {" 1:00 Passkeys", VERT_ITEM, TEXT, {NULL}},
   {" 1:00 Cyber Game", VERT_ITEM, TEXT, {NULL}},
   {" 1:00 Tradecraft", VERT_ITEM, TEXT, {NULL}},
   {" 1:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 TTP", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 Enterprise", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 SW BoB", VERT_ITEM, TEXT, {NULL}},
   {" 2:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 AD pentest", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 pandemic", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 Threat Hnt", VERT_ITEM, TEXT, {NULL}},
   {" 3:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 4:00 NW 201", VERT_ITEM, TEXT, {NULL}},
   {" 4:00 No Popo", VERT_ITEM, TEXT, {NULL}},
   {" 4:50 Closing", VERT_ITEM, TEXT, {NULL}},
   {" 5:00 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 5:30 Aft Party", VERT_ITEM, TEXT, {NULL}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t schedule_m[] = {
   {"Tuesday", VERT_ITEM|DEFAULT_ITEM, MENU, {day1_p1_m}},
   {"Wednesday", VERT_ITEM, MENU, {day2_p1_m}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

