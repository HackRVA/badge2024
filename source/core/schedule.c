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
   {" 7:59 Registrat", VERT_ITEM, ITEM_DESC,
	{ .description =
		"If you were\n"
		"not able to\n"
		"attend Day 1,\n"
		"please proceed\n"
		"upstairs to\n"
		"register.\n\n"
		"Top of the\n"
		"Grand\n",
	},
   },
   {" 8:00 Breakfast", VERT_ITEM, MENU, {day2_breakfast_m}},
   {" 8:50 Welcome", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Welcome to Day\n"
		"2 RVAsec 12!\n\n"
		"Remarks will be\n"
		"provided about\n"
		"what to expect\n"
		"at the conference\n\n"
		"Ballroom",
	},
   },
   {" 9:00 Keynote", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Keynote:\n"
		"Building\n"
		"Leadership, 1%\n"
		"at a time\n\n"
		"Andy Ellis\n\n"
		"Ballroom\n",
	},
   },
   {"10:00 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"10:00 CTF Comp", VERT_ITEM, ITEM_DESC,
	{ .description =
		"CTF Competition\n"
		"10-3pm\n\n"
		"Shenandoah Room",
	},
   },
   {"10:00 Badge", VERT_ITEM, ITEM_DESC,
	{.description =
		"Badge Repair\n\n"
		"Come learn about\n"
		"your badge, get\n"
		"it fixed if\n"
		"there are any\n"
		"issues and talk\n"
		"to HackRVA!\n\n"
		"Rappahannock",
	},
   },
   {"10:00 Lock Pick", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Lock Picking\n"
		"Village and\n"
		"Contest\n\n"
		"A variety of\n"
		"locks, from\n"
		"simple to very\n"
		"hard, along\n"
		"with picks\n"
		"of all kinds.\n"
		"Test your\n"
		"lock picking\n"
		"skills.\n\n"
		"Rappahannock",
	},
   },
   {"10:30 NSA Cryp", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Shakespeare,\n"
		"Bacon, and\n"
		"the NSA:\n\n"
		"The peculiar\n"
		"story of the\n"
		"history of\n"
		"cryptography -\n"
		"featuring a\n"
		"code-breaking\n"
		"Quaker poet\n\n"
		"Ballroom C/D",
	},
   },
   {"10:30 CISO", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Top 5 CISO\n"
		"Findings of 2022\n\n"
		"Ballroom A/B",
	},
   },
   {"10:30 Heap Exp", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Heap Exploi-\n"
		"tation from\n"
		"First Principles\n"
		"building a\n"
		"userland heap\n"
		"allocator,\n"
		"identifying\n"
		"weaknesses in\n"
		"heap allocation\n"
		"and demoing ways\n"
		"to exploit such\n"
		"weaknesses.\n\n"
		"1st Floor\n"
		"Magnolia Room",
	},
   },
   {"11:20 -Break-", VERT_ITEM, TEXT, {NULL}},
   {"11:30 NIST/CMMC", VERT_ITEM, ITEM_DESC,
	{ .description =
		"The State of\n"
		"NIST/CMMC\n"
		"Compliance\n"
		"Today\n\n"
		"1st Floor\n"
		"Magnolia Room",
	},
   },
   {"11:30 Leadrshp", VERT_ITEM, ITEM_DESC,
	{ .description =
		"This Is The\n"
		"Way: A New\n"
		"Leadership\n"
		"Creed for\n"
		"Info-Sec\n"
		"Professionals\n\n"
		"Ballroom C/D",
	},
   },
   {"11:30 Intruder", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Who Goes\n"
		"There?\n"
		"Actively\n"
		"Detecting\n"
		"Intruders With\n"
		"Cyber\n"
		"Deception Tools\n\n"
		"Ballroom A/B",
	},
   },
   {"12:20 Lunch", VERT_ITEM, MENU, {day2_lunch_m}},
   {" 1:00 Quantum", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Quantum\n"
		"Cybersecurity\n\n"
		"Implications\n"
		"of the advent\n"
		"of quantum\n"
		"computers\n\n"
		"Ballroom A/B",
	},
   },
   {" 1:00 Cmnwealth", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Cyber, the\n"
		"Commonwealth\n"
		"and You\n\n"
		"Whole Govt\n"
		"approach to\n"
		"cyber\n\n"
		"Ballroom C/D",
	},
   },
   {" 1:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 ChatGPT", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Hacking Your\n"
		"Job? Trying To\n"
		"Cheat At Life\n"
		"With ChatGPT\n\n"
		"Ballroom C/D",
	},
   },
   {" 2:00 Ransomwre", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Ransomware\n"
		"Rebranding\n"
		"... So Hot\n"
		"Right Now!\n\n"
		"Ballroom A/B",
	},
   },
   {" 2:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:10 Insiders", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Insiders\n"
		"Packing Their\n"
		"Bags With\n"
		"Your Data\n\n"
		"Which of\n"
		"your employees\n"
		"are\n"
		"exfiltrating\n"
		"data prior to\n"
		"leaving?\n\n"
		"Ballroom",
	},
   },
   {" 4:00 Closing", VERT_ITEM, ITEM_DESC,
	{ .description =
		"The closing\n"
		"will occur\n"
		"after the\n"
		"final talk.\n\n"
		"After a short\n"
		"break for\n"
		"beverages &\n"
		"Hors d'oeuvres,\n"
		"we will award\n"
		"Prizes and\n"
		"CTF awards\n\n"
		"Ballroom",
	},
   },
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
   {" 7:59 Registrat", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Registration\n\n"
		"1st Floor\n"
		"Magnolia Room\n\n"
		"After the\n"
		"initial rush\n"
		"of registra-\n"
		"tion in the\n"
		"morning, it\n"
		"will be\n"
		"relocated to\n"
		"the \"Top of\n"
		"the Grand\".\n",
	},
   },
   {" 8:00 Breakfast", VERT_ITEM, MENU, {day1_breakfast_m}},
   {" 9:00 Welcome", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Welcome to\n"
		"RVAsec 12!\n\n"
		"Remarks will be\n"
		"provided about\n"
		"what to expect\n"
		"at the\n"
		"conference.\n"
		"We will have\n"
		"short presenta-\n"
		"tions on CTF,\n"
		"Badge, and\n"
		"Lock Picking.\n\n"
		"Ballroom",
	},
   },
   {" 9:30 Keynote", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Keynote Speaker\n\n"
		"Paul Asadoorian\n"
		"  Security\n"
		"  Evangelist\n"
		"Eclypsium\n\n"
		"Ballroom",
	},
   },
   {"10:00 Badge", VERT_ITEM, ITEM_DESC,
	{.description =
		"Badge Repair\n\n"
		"Come learn about\n"
		"your badge, get\n"
		"it fixed if\n"
		"there are any\n"
		"issues and talk\n"
		"to HackRVA!\n\n"
		"Rappahannock",
	},
   },
   {"10:00 Lock Pick", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Lock Picking\n"
		"Village and\n"
		"Contest\n\n"
		"A variety of\n"
		"locks, from\n"
		"simple to very\n"
		"hard, along\n"
		"with picks\n"
		"of all kinds.\n"
		"Test your\n"
		"lock picking\n"
		"skills.\n\n"
		"Rappahannock",
	},
   },
   {"10:30 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Vendor Break\n"
		"& Room Change\n\n"
		"All attendees\n"
		"need to leave\n"
		"both ballrooms\n"
		"so we can\n"
		"split the\n"
		"room for\n"
		"talks.",
	}
   },
   {"11:00 Liability", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Cybernation:\n"
		"The FUD, Facts\n"
		"and Future of\n"
		"Software\n"
		"Liability and\n"
		"Security\n\n"
		"Ballroom A/B"
	},
   },
   {"11:00 Passwd", VERT_ITEM, ITEM_DESC,
	{ .description =
		"I Heart My\n"
		"Password\n\n"
		"Protecting\n"
		"identity is\n"
		"foundational\n"
		"to zero trust,\n"
		"and everybody\n"
		"wants\n"
		"passwordless,\n"
		"but is it\n"
		"always\n"
		"appropriate?\n\n"
		"Ballroom C/D"
	},
   },
   {"11:50 Lunch", VERT_ITEM, MENU, {day1_lunch_m}},
   {" 1:00 Passkeys", VERT_ITEM, ITEM_DESC,
	{. description =
		"Everything You\n"
		"Never Knew You\n"
		"Wanted To Know\n"
		"About Passkeys\n\n"
		"1st floor\n"
		"Magnolia Room\n",
	},
   },
   {" 1:00 CyberGame", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Corporate\n"
		"Dungeon\n"
		"Master: How\n"
		"to Lead Cyber\n"
		"Games at Work\n\n"
		"Ballroom C/D",
	},
   },
   {" 1:00 Tradecrft", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Context Matters:\n"
		"Tailoring Trade-\n"
		"craft to the\n"
		"Operational\n"
		"Environment\n\n"
		"Ballroom A/B",
	},
   },
   {" 1:00 CTF Prep", VERT_ITEM, ITEM_DESC,
	{ .description =
		"CTF Prep\n\n"
		"Come prep and\n"
		"learn more about\n"
		"the CTF contest!\n\n"
		"Shenandoah Room\n",
	},
   },
   {" 1:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 2:00 TTP", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Adversary\n"
		"tactics\n"
		"techniques and\n"
		"procedures (TTPs)\n"
		"Evolution &\n"
		"the Value of\n"
		"TTP Intelligence\n\n"
		"Ballroom A/B",
	},
   },
   {" 2:00 Enterpris", VERT_ITEM, ITEM_DESC,
	{ .description =
		"A programmatic\n"
		"approach to\n"
		"enterprise\n"
		"security OR\n"
		"How to not\n"
		"waste your\n"
		"security budget\n"
		"on sh!7 that\n"
		"doesn't matter!\n\n"
		"Ballroom C/D",
	},
   },
   {" 2:00 SW BoB", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Software Bills\n"
		"of Behaviors:\n"
		"Why SBOMs\n"
		"Aren't Enough\n\n"
		"1st floor\n"
		"Magnolia Room",
	},
   },
   {" 2:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 3:00 Pentest", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Feature or a\n"
		"Vulnerability?\n"
		"Tale of an\n"
		"Active\n"
		"Directory\n"
		"Pentest\n\n"
		"Ballroom C/D",
	},
   },
   {" 3:00 Pandemic", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Beyond The\n"
		"Pandemic: How\n"
		"The Pandemic\n"
		"Shaped\n"
		"Organizations\n"
		"and Their\n"
		"Security\n"
		"Architecture\n\n"
		"1st Floor\n"
		"Magnolia Room",
	},
   },
   {" 3:00 ThreatHnt", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Maturing Your\n"
		"Threat Hunting\n"
		"Operations:\n"
		"a roadmap for\n"
		"designing a\n"
		"mature threat\n"
		"hunting service.\n\n"
		"Ballroom A/B",
	},
   },
   {" 3:50 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 4:00 NW 201", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Network 201:\n"
		"A Tour Through\n"
		"Network Security\n\n"
		"1st Floor\n"
		"Magnolia Room",
	},
   },
   {" 4:00 No Popo", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Why You Can't\n"
		"Call the Police\n\n"
		"Walk through\n"
		"investigating\n"
		"crypto hot\n"
		"wallets and\n"
		"NFTs while we\n"
		"collect\n"
		"electronic\n"
		"evidence with\n"
		"proper chain\n"
		"of custody to\n"
		"prove a theft\n"
		"occurred.\n\n"
		"Ballroom A/B",
	},
   },
   {" 4:50 Closing", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Remarks will\n"
		"be provided\n"
		"on Day 1, and\n"
		"what to expect\n"
		"for the rest\n"
		"of the evening\n"
		"and Day 2.",
	},
   },
   {" 5:00 -Break-", VERT_ITEM, TEXT, {NULL}},
   {" 5:30 Aft Party", VERT_ITEM, ITEM_DESC,
	{ .description =
		"RVAsec After\n"
		"Party\n\n"
		"The RVAsec 2023\n"
		"after party,\n"
		"brought to you\n"
		"by RVAsec, and\n"
		"will be in the\n"
		"main Ballroom,\n"
		"right after the\n"
		"conference ends!\n"
    		"5:30 pm to\n"
		"8:30pm: Heavy\n"
		"hors d'oeuvre\n"
		"(new menu!)\n"
		"and beverages",
	},
   },
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t schedule_m[] = {
   {"Tuesday", VERT_ITEM|DEFAULT_ITEM, MENU, {day1_p1_m}},
   {"Wednesday", VERT_ITEM, MENU, {day2_p1_m}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

