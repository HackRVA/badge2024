#include "menu.h"

#ifndef NULL
#define NULL 0
#endif

const struct menu_t day2_p1_m[] = {
   {"Wednesday", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}, NULL},
   {" 7:59 Registrat", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Registration\n\n"
		"7:59-5:00\n"
		"Upstairs, Desk\n"
	},
	NULL,
   },
   {" 8:00 Breakfast", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Breakfast\n\n"
		"8:00-8:50\n"
		"Downstairs,\n"
		"Foyer\n",
	},
	NULL,
   },
   {" 9:00 Welcome", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Welcome to Day\n"
		"2 RVAsec 13!\n\n"
		"8:50-9:00\n"
		"Grand\n"
		"Ballroom\n"
		"D/E/F/G\n"
		"Jake Kouns",
	},
	NULL,
   },
   {" 9:00 Keynote", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Keynote\n\n"
		"9:00-10:00\n"
		"Grand\n"
		"Ballroom\n"
		"D/E/F/G\n"
		"Caleb Sima",
	},
	NULL,
   },
   {"10:00 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Vendor Break\n\n"
		"10:00-10:30\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom",
	},
	NULL,
   },
   {"10:00 CTF Comp", VERT_ITEM, ITEM_DESC,
	{ .description =
		"CTF Competition\n\n"
		"10:00am-3pm\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom\n"
		"Middle",
	},
	NULL,
   },
   {"10:00 Badge", VERT_ITEM, ITEM_DESC,
	{.description =
		"Badge Training\n"
		"and Repair\n\n"
		"10:30am-4:30pm\n"
		"Downstairs,\n"
		"Dominion\n\n"
		"Come learn about\n"
		"your badge, get\n"
		"it fixed if\n"
		"there are any\n"
		"issues and talk\n"
		"to HackRVA!",
	},
	NULL,
   },
   {"10:00 Lock Pick", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Lock Picking\n"
		"Village and\n"
		"Contest\n\n"
		"10:30am-5pm\n"
		"Shenandoah\n\n"
		"A variety of\n"
		"locks, from\n"
		"simple to very\n"
		"hard, along\n"
		"with picks\n"
		"of all kinds.\n"
		"Test your\n"
		"lock picking\n"
		"skills.",
	},
	NULL,
   },
   {"10:30 AI Chat", VERT_ITEM, ITEM_DESC,
	{ .description =
		"QuickStart To\n"
		"Building Your\n"
		"Own Private AI\n"
		"Chat\n\n"
		"10:30-11:20\n"
		"Downstairs\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"Samuel Panicker",
	},
	NULL,
   },
   {"10:30 Apples", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Apples to\n"
		"Apples\n\n"
		"10:30-11:20\n"
		"Grand\n"
		"Ballroom\n"
		"D/E/F/G\n"
		"Pyr0 (Luke\n"
		"McOmie)",
	},
	NULL,
   },
   {"10:30 Patch", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Patch Perfect:\n"
		"Harmonizing\n"
		"with LLMs to\n"
		"Find Security\n"
		"Vulns\n\n" 
		"10:30-11:20\n"
		"Grand\n"
		"Ballroom\n"
		"F/G\n"
		"Josh Shomo\n"
		"Caleb Gross",
	},
	NULL,
   },
   {"11:20 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Vendor Break\n\n"
		"11:20-11:30\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom\n"
	},
	NULL,
   },
   {"11:30 Mindful", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Mindfulness,\n"
		"Meditation,\n"
		"and Cyber\n"
		"Security\n\n"
		"11:30-12:20\n"
		"Downstairs\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"Aqeel Yaseen",
	},
	NULL,
   },
   {"11:30 GRC", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Strategic\n"
		"Alliances:\n"
		"How GRC Teams\n"
		"Can Empower\n"
		"Offensive\n"
		"Security\n"
		"Effors\n\n"
		"11:30-12:20\n"
		"Upstairs,\n"
		"Grand\n"
		"Ballroom\n"
		"D/E\n"
		"Darryl MacLeod",
	},
	NULL,
   },
   {"11:30 Gen AI", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Unlocking\n"
		"Generative\n"
		"AI: Balancing\n"
		"Innovation with\n"
		"Security\n\n"
		"11:30-12:20\n"
		"Upstairs,\n"
		"Grand\n"
		"Ballroom\n"
		"F/G\n"
		
	},
	NULL,
   },
   {"12:20 Lunch", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Lunch\n\n"
		"12:20-1:00\n"
		"Downstairs,\n"
		"Foyer\n",
	},
	NULL,
   },
   {" 1:00 SocEng", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Social\n"
		"Engineering\n"
		"the Social\n"
		"Engineers:\n"
		"How To Not\n"
		"Suck At Buying\n"
		"Software\n\n"
		"1:00-1:50\n"
		"Downstairs\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"David Girvin",
	},
	NULL,
   },
   {" 1:00 Human Exp", VERT_ITEM, ITEM_DESC,
	{ .description =
		"The Human\n"
		"Experience of\n"
		"Security\n"
		"Operations\n\n"
		"1:00-1:50\n"
		"Upstairs,\n"
		"Grand\n"
		"Ballroom\n"
		"D/E\n"
		"Chris Tillett",
	},
	NULL,
   },
   {" 1:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Vendor Break\n"
		"1:50-2:00\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom\n",
	},
	NULL,
   },
   {" 2:00 API", VERT_ITEM, ITEM_DESC,
	{ .description =
		"API-ocalypse\n\n"
		"2:00-2:50\n"
		"Downstairs\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"Jennifer Shannon",
	},
	NULL,
   },
   {" 2:00 Creative", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Scaling Your\n"
		"Creative\n"
		"Output with AI:\n"
		"Lessons Learned\n"
		"from SANS\n"
		"Holiday Hack\n"
		"Challenge\n\n"
		"2:00-2:50\n"
		"Upstairs,\n"
		"Grand\n"
		"Ballroom\n"
		"D/E\n"
		"Evan Booth",
	},
	NULL,
   },
   {" 2:00 Illusions", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Building\n"
		"Illusions in\n"
		"the Cloud:\n"
		"Deception\n"
		"Engineering\n\n"
		"2:00-2:50\n"
		"Upstairs,\n"
		"Grand\n"
		"Ballroom\n"
		"F/G\n"
		"Ayush Priya\n"
		"Saksham Tushar",
	},
	NULL,
   },
   {" 2:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Vendor Break\n\n"
		"2:50-3:10\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom\n",
	},
	NULL,
   }, 
   {" 3:10 Shaping", VERT_ITEM, ITEM_DESC,
	{ .description =
		"My Way Is Not\n"
		"Very\n"
		"Sportsmanlike:\n"
		"Shaping Adversary\n"
		"Behavior to\n"
		"Strengthen\n"
		"Defenses\n\n"
		"3:10-4:00\n"
		"Upstairs,\n"
		"Grand\n"
		"Ballroom\n"
		"F/G\n"
		"David J. Bianco",
	},
	NULL,
   },
   {" 4:00 Closing", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Closing\n"
		"Reception\n"
		"and Awards\n\n"
		"4:00-5:30\n"
		"Grand\n"
		"Ballroom\n"
		"D/E/F/G\n"
		"Chris Sullo",
	},
	NULL,
   },
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}, NULL},
};

const struct menu_t day1_p1_m[] = {
   {"Tuesday", VERT_ITEM|SKIP_ITEM, TEXT, {NULL}, NULL},
   {" 7:59 Registrat", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Upstairs, Desk\n",
	},
	NULL,
   },
   {" 8:00 Breakfast", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Breakfast\n\n"
		"8:00-9:00\n"
		"Downstairs,\n"
		"Foyer\n",
	},
	NULL,
   },
   {" 9:00 Welcome", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Welcome to\n"
		"RVAsec 13!\n\n"
		"9:00-9:30\n"
		"Upstairs, Grand\n"
		"Ballroom\n"
		"Jake Kouns,\n"
		"Peter Maxwell\n"
		" Warsila,\n"
		"Roman Bohuk",
	},
	NULL,
   },
   {" 9:30 Keynote", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Keynote Speaker\n\n"
		"9:30-10:30\n"
		"Upstairs, Grand\n"
		"Ballroom\n"
		"Kymberlee Price",
	},
	NULL,
   },
   {"10:30 Break", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Vendor Break\n"
		"Room Change\n\n"
		"10:30-11:00",
	},
	NULL,
   },
   {"10:30 Badge", VERT_ITEM, ITEM_DESC,
	{.description =
		"Badge Training\n"
		"and Repair\n\n"
		"10:30am-4:30pm\n"
		"Downstairs,\n"
		"Dominion\n\n"
		"Come learn about\n"
		"your badge, get\n"
		"it fixed if\n"
		"there are any\n"
		"issues and talk\n"
		"to HackRVA!",
	},
	NULL,
   },
   {"10:30 Lock Pick", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Lock Picking\n"
		"Village and\n"
		"Contest\n\n"
		"10:30am-5pm\n"
		"Shenandoah\n\n"
		"A variety of\n"
		"locks, from\n"
		"simple to very\n"
		"hard, along\n"
		"with picks\n"
		"of all kinds.\n"
		"Test your\n"
		"lock picking\n"
		"skills.",
	},
	NULL,
   },
   {"11:00 Payload", VERT_ITEM, ITEM_DESC,
	{ .description =
		"That Shouldn't\n"
		"Have Worked - \n"
		"Payload\n"
		"Development\n\n"
		"11-11:50\n"
		"Upstairs\n"
		"Grand Ballroom\n"
		"Corey Overstreet",
	},
	NULL,
   },
   {"11:50 Lunch", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Lunch\n\n"
		"11:50-1:00\n"
		"Downstairs,\n"
		"Foyer",
	},
	NULL,
   },
   {" 1:00 Phy Pntst", VERT_ITEM, ITEM_DESC,
	{. description =
		"It's Coming\n"
		"From Inside the\n"
		"House: A Guide\n"
		"to Physical\n"
		"Facility\n"
		"Pen Testing\n"
		"1:00-1:50\n"
		"Downstairs\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"Ariyan Suroosh",
	},
	NULL,
   },
   {" 1:00 Risk Rem", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Verified for\n"
		"Business\n"
		"Continuity:\n"
		"How to Re-\n"
		"mediate Risk\n"
		"Safely across\n"
		"the Enterprise\n"
		"1:00-1:50\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"D/E\n"
		"Oren Koren"
	},
	NULL,
   },
   {" 1:00 RCE Elec", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Some Assembly\n"
		"Required: Weap-\n"
		"onizing Chrome\n"
		"CVE-2023-2033\n"
		"for RCE in\n"
		"Electron\n"
		"1:00-1:50\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"F/G\n"
		"Nick Copi",
	},
	NULL,
   },
   {" 1:00 CTF Prep", VERT_ITEM, ITEM_DESC,
	{ .description =
		"CTF Prep\n\n"
		"Come prep and\n"
		"learn more about\n"
		"the CTF contest!\n\n"
		"1:00-4:00\n"
		"Downstairs,\n"
		"Capitol\n"
		"Ballroom\n"
		"Middle",
	},
	NULL,
   },
   {" 1:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"1:50-2:00\n"
		"Downstairs,\n"
		"Capitol\n"
		"Ballroom",
	},
	NULL,
   },
   {" 2:00 DevSecOps", VERT_ITEM, ITEM_DESC,
	{ .description =
		"The ABCs of\n"
		"DevSecOps\n\n"
		"2:00-2:50\n"
		"Downstairs,\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"Steve Pressman",
	},
	NULL,
   },
   {" 2:00 APT Aware", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Once Upon a\n"
		"Cyber Threat:\n"
		"The Brothers\n"
		"Grimm's\n"
		"Teachings on\n"
		"APT Awareness\n\n"
		"2:00-2:50\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"D/E\n"
		"Ell Marquez",
	},
	NULL,
   },
   {" 2:00 Routers", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Consumer\n"
		"Routers\n"
		"Still Suck\n\n"
		"2:00-2:50\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"F/G\n"
		"Evan Grant\n"
		"Jim Sebree",
	},
	NULL,
   },
   {" 2:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"2:50-3:00\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom",
	},
	NULL,
   },
   {" 3:00 Impostor", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Embracing My\n"
		"Inner Cyber\n"
		"Wizard To\n"
		"Defeat\n"
		"Impostor\n"
		"Syndrome\n\n"
		"3:00-3:50\n"
		"Downstairs\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"Corey Brennan",
	},
	NULL,
   },
   {" 3:00 AI Advers", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Defending\n"
		"Against the\n"
		"Deep: Is Your\n"
		"Workforce Ready\n"
		"For Generative\n"
		"AI Adversaries\n\n"
		"3:00-3:50\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"D/E\n"
		"Tucker Mahan",
	},
	NULL,
   },
   {" 3:00 Hack Exch", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Hacking\n"
		"Exchange\n"
		"From The\n"
		"Outside In\n\n"
		"3:00-3:50\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"F/G\n"
		"Ali Ahmad",
	},
	NULL,
   },
   {" 3:50 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Break\n\n"
		"3:50-4:00\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"F/G\n",
	},
	NULL,
    },
   {" 4:00 Improv", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Improv\n"
		"Comedy for\n"
		"Social\n"
		"Engineering\n\n"
		"4:00-4:50\n"
		"Downstairs\n"
		"Madison/\n"
		"Jefferson/\n"
		"Monroe\n"
		"Ross Merritt",
	},
	NULL,
   },
   {" 4:00 PenTest", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Orion's Quest:\n"
		"Navigating\n"
		"the Cyber\n"
		"Wilderness -\n"
		"Tales of Modern\n"
		"Penetration\n"
		"Testing\n\n"
		"4:00-4:50\n"
		"Upstairs,\n"
		"Grand Ballroom\n"
		"F/G\n"
		"Kevin Johnson",
	},
	NULL,
   },
   {" 4:50 Closing", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Closing\n\n"
		"4:50-5:00\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom"
		"Jake Kouns\n"
		"Chris Sullo",
	},
	NULL,
   },
   {" 5:00 -Break-", VERT_ITEM, ITEM_DESC,
	{ .description =
		"Vendor Break\n"
		"and Room Change\n\n"
		"5:00-5:30\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom",
	},
	NULL,
   },
   {" 5:30 Aft Party", VERT_ITEM, ITEM_DESC,
	{ .description =
		"RVAsec After\n"
		"Party\n\n"
		"5:30-9:00\n"
		"Downstairs\n"
		"Capitol\n"
		"Ballroom",
	},
	NULL,
   },
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}, NULL},
};


const struct menu_t schedule_m[] = {
   {"Tuesday", VERT_ITEM|DEFAULT_ITEM, MENU, {day1_p1_m}, NULL},
   {"Wednesday", VERT_ITEM, MENU, {day2_p1_m}, NULL},
   {"back", VERT_ITEM|LAST_ITEM, BACK, {NULL}, NULL},
};

