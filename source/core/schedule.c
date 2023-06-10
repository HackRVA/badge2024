#include "menu.h"

#ifndef NULL
#define NULL 0
#endif

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
		"Grand\n\n"
		"7:59-5:00\n"
		"Top of the\n"
		"Grand"
	},
   },
   {" 8:00 Breakfast", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Breakfast\n\n"
		"Bacon/Egg Bagel\n"
		"  sandwich\n"
		"GF Wraps w/ egg\n"
		"  whites, spinach\n"
		"  peppers & onion\n"
		"Hashbrowns\n"
		"Doughnut Holes\n"
		"Danishes\n"
		"Fruit\n\n"
		"8:00-8:50\n"
		"Top of the\n"
		"Grand",
	},
   },
   {" 8:50 Welcome", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Welcome to Day\n"
		"2 RVAsec 12!\n\n"
		"Remarks will be\n"
		"provided about\n"
		"what to expect\n"
		"at the conference\n\n"
		"8:50-9:00\n"
		"Ballroom\n"
		"Jake Kouns",
	},
   },
   {" 9:00 Keynote", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Keynote:\n"
		"Building\n"
		"Leadership, 1%\n"
		"at a time\n\n"
		"9:00-10:00\n"
		"Ballroom\n"
		"Andy Ellis",
	},
   },
   {"10:00 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"Coffee Cake\n"
		"Peach & Apple\n"
		"  Cobbler\n\n"
		"10:00-10:30\n"
		"Potomac",
	},
   },
   {"10:00 CTF Comp", VERT_ITEM, ITEM_DESC,
	{ .description =
		"CTF Competition\n\n"
		"10:00am-3pm\n"
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
		"10:00-4:00\n"
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
		"10:00-4:00\n"
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
		"10:30-11:20\n"
		"Ballroom C/D\n"
		"Brendan O'Leary",
	},
   },
   {"10:30 CISO", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Top 5 CISO\n"
		"Findings of 2022\n\n"
		"10:30-11:20\n"
		"Ballroom A/B\n"
		"Mark Arnold",
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
		"10:30-11:20\n"
		"1st Floor\n"
		"Magnolia Room\n"
		"Kevin Massey",
	},
   },
   {"11:20 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"Coffee Cake\n"
		"Peach & Apple\n"
		"  Cobbler\n\n"
		"11:20-11:30\n"
		"Potomac\n",
	},
   },
   {"11:30 NIST/CMMC", VERT_ITEM, ITEM_DESC,
	{ .description =
		"The State of\n"
		"NIST/CMMC\n"
		"Compliance\n"
		"Today\n\n"
		"11:30-12:20\n"
		"1st Floor\n"
		"Magnolia Room\n"
		"Ian MacRae",
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
		"11:30-12:20\n"
		"Ballroom C/D\n"
		"Kate Collins\n",
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
		"11:30-12:20\n"
		"Ballroom A/B\n"
		"Dwayne McDaniel",
	},
   },
   {"12:20 Lunch", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Box Lunch\n"
		"Choice of:\n"
		"1.Chicken Salad\n"
		"  on croissant\n"
		"2.Club Sandwich\n"
		"Plus\n"
		" Potato Salad\n"
		" Fruit Cup\n"
		" Lemon Bar\n"
		" Condiments\n"
		" Utensils\n"
		"OR\n"
		"GF Tortilla\n"
		"Wrap w/ Asparagus,\n"
		"Spinach, Bell\n"
		"Pepper, Tomato\n"
		"Onion and Garlic\n"
		"Hummus\n"
		"12:20-1:00\n"
		"James River Foyer",
	},
   },
   {" 1:00 Quantum", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Quantum\n"
		"Cybersecurity\n\n"
		"Implications\n"
		"of the advent\n"
		"of quantum\n"
		"computers\n\n"
		"1:00-1:50\n"
		"Ballroom A/B\n"
		"Denis Mandich",
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
		"1:00-1:50\n"
		"Ballroom C/D\n"
		"Aliscia Andrews",
	},
   },
   {" 1:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Pretzels w/\n"
		"  Cheese Dip &\n"
		"  Mustard\n"
		"Cinnamon Sugar\n"
		"  Pretzels\n\n"
		"1:50-2:00\n"
		"Potomac",
	},
   },
   {" 2:00 ChatGPT", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Hacking Your\n"
		"Job? Trying To\n"
		"Cheat At Life\n"
		"With ChatGPT\n\n"
		"2:00-2:50\n"
		"Ballroom C/D\n"
		"David Girvin\n",
	},
   },
   {" 2:00 Ransomwre", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Ransomware\n"
		"Rebranding\n"
		"... So Hot\n"
		"Right Now!\n\n"
		"2:00-2:50\n"
		"Ballroom A/B\n"
		"Drew Schmitt",
	},
   },
   {" 2:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"Pretzels w/\n"
		"  Cheese Dip &\n"
		"  Mustard\n"
		"Cinnamon Sugar\n"
		"Pretzels\n\n"
		"2:50-3:10\n"
		"Potomac",
	},
   }, 
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
		"3:10-4:00\n"
		"Ballroom\n"
		"Colin Estep",
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
		"4:00-5:30\n"
		"Ballroom\n"
		"Chris Sullo",
	},
   },
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

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
   {" 8:00 Breakfast", VERT_ITEM, ITEM_DESC,
	{ .description =
		"VA Baked/Fried\n"
		" Ham Biscuits\n"
		"Mini Veggie\n"
		" Frittatas\n"
		" GF/Veg/nondairy\n"
		"Cheese Grits w/\n"
		" butter\n"
		" GF/Veg\n"
		"Muffins\n"
		" Cranberry, Orange\n"
		" Walnut, Blueberry\n"
		" Chocolate\n"
		"Seasonal Fruit\n"
		" GF/Vegan\n"
		" non-dairy\n\n"
		"11-50-1:00\n"
		"James River Foyer",
	},
   },
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
		"Ballroom\n"
		"Jake Kouns"
	},
   },
   {" 9:30 Keynote", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Keynote Speaker\n\n"
		"Paul Asadoorian\n"
		"  Security\n"
		"  Evangelist\n"
		"Eclypsium\n\n"
		"9:30-10:30\n"
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
		"10:00am-4:30pm\n"
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
		"10am-5pm\n"
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
		"talks.\n\n"
		"Mini Cinn Buns\n"
		"Yogurt Parfait\n"
		"10:30-11:00\n"
		"Potomac",
	},
   },
   {"11:00 Liability", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Cybernation:\n"
		"The FUD, Facts\n"
		"and Future of\n"
		"Software\n"
		"Liability and\n"
		"Security\n\n"
		"Ballroom A/B\n"
		"11-11:50\n"
		"Andrea Matwyshyn",
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
		"Ballroom C/D\n"
		"11-11:50\n"
		"Adrian Amos",
	},
   },
   {"11:50 Lunch", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Box Lunch\n\n"
		"Choose one:\n"
		"1.Shredded BBQ\n"
		"  Chicken\n"
		"2.Diced Port-\n"
		"  abello Mush-\n"
		"  rooms in GF\n"
		"  BBQ Sauce\n"
		"Plus\n"
		"  Roll, Coleslaw,\n"
		"  Chips, Brownie,\n" 
		"  hot sauce,\n"
		"  utensils\n"
		"  napkins\n"
		"11:50-1:00\n"
		"James River Foyer",
	},
   },
   {" 1:00 Passkeys", VERT_ITEM, ITEM_DESC,
	{. description =
		"Everything You\n"
		"Never Knew You\n"
		"Wanted To Know\n"
		"About Passkeys\n\n"
		"1:00-1:50\n"
		"1st floor\n"
		"Magnolia Room\n"
		"Josh Cigna",
	},
   },
   {" 1:00 CyberGame", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Corporate\n"
		"Dungeon\n"
		"Master: How\n"
		"to Lead Cyber\n"
		"Games at Work\n\n"
		"1:00-1:50\n"
		"Ballroom C/D\n"
		"Jason Wonn"
	},
   },
   {" 1:00 Tradecrft", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Context Matters:\n"
		"Tailoring Trade-\n"
		"craft to the\n"
		"Operational\n"
		"Environment\n\n"
		"1:00-1:50\n"
		"Ballroom A/B\n"
		"Fletcher Davis",
	},
   },
   {" 1:00 CTF Prep", VERT_ITEM, ITEM_DESC,
	{ .description =
		"CTF Prep\n\n"
		"Come prep and\n"
		"learn more about\n"
		"the CTF contest!\n\n"
		"1:00-4:00\n"
		"Shenandoah Room\n",
	},
   },
   {" 1:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"Milk and Cookies\n"
		"Cold Milk\n"
		"Chocolate Milk\n\n"
		"1:50-2:00\n"
		"Potomac\n",
	},
   },
   {" 2:00 TTP", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Adversary\n"
		"tactics\n"
		"techniques and\n"
		"procedures (TTPs)\n"
		"Evolution &\n"
		"the Value of\n"
		"TTP Intelligence\n\n"
		"2:00-2:50\n"
		"Ballroom A/B\n"
		"Scott Small",
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
		"2:00-2:50\n"
		"Ballroom C/D",
	},
   },
   {" 2:00 SW BoB", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Software Bills\n"
		"of Behaviors:\n"
		"Why SBOMs\n"
		"Aren't Enough\n\n"
		"2:00-2:50\n"
		"1st floor\n"
		"Magnolia Room\n"
		"Andrew Hendela",
	},
   },
   {" 2:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"Potato Chips &\n"
		"French Onion\n"
		"  Dip\n\n"
		"2:50-3:00\n"
		"Potomac"
	},
   },
   {" 3:00 Pentest", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Feature or a\n"
		"Vulnerability?\n"
		"Tale of an\n"
		"Active\n"
		"Directory\n"
		"Pentest\n\n"
		"3:00-3:50\n"
		"Ballroom C/D\n"
		"Qasim Ijaz",
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
		"3:00-3:50\n"
		"1st Floor\n"
		"Magnolia Room\n"
		"Dan Han",
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
		"3:00-3:50\n"
		"Ballroom A/B\n"
		"Andrew Skatoff",
	},
   },
   {" 3:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"Potato Chips &\n"
		"French Onion\n"
		"  Dip\n\n"
		"3:50-4:00\n"
		"Potomac",
	},
    },
   {" 4:00 NW 201", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Network 201:\n"
		"A Tour Through\n"
		"Network Security\n\n"
		"4:00-4:50\n"
		"1st Floor\n"
		"Magnolia Room\n"
		"Rick Lull\n",
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
		"4:00-4:50\n"
		"Ballroom A/B\n"
		"Amelia Szczuchniak",
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
		"and Day 2.\n\n"
		"4:50-5:00\n"
		"Jake Kouns\n"
		"Chris Sullo",
	},
   },
   {" 5:00 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"Veg Crudite\n"
		"Ranch Dip\n"
		"Spinach/Artichoke\n"
		"  Dip\n"
		"Pita Chips\n"
		"Crackers\n\n"
		"5:00-5:30\n"
		"James River Foyer\n",
	},
   },
   {" 5:30 Aft Party", VERT_ITEM, ITEM_DESC,
	{ .description =
		"RVAsec After\n"
		"Party\n\n"
		"Casino Night!\n"
		"-Food\n"
		"-Beverages\n"
		"-Games 5:30-8:30\n\n"
		"5:30-9:00\n"
		"Omni Ballroom"
	},
   },
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};


const struct menu_t schedule_m[] = {
   {"Tuesday", VERT_ITEM|DEFAULT_ITEM, MENU, {day1_p1_m}},
   {"Wednesday", VERT_ITEM, MENU, {day2_p1_m}},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};

