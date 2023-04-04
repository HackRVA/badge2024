#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "random.h"
#include "audio.h"
#include "led_pwm.h"
#include "assetList.h"
#include "key_value_storage.h"

/* Program states.  Initial state is MYPROGRAM_INIT */
static enum slot_machine_state_t
{
	SLOT_MACHINE_INIT,
	SLOT_MACHINE_BET,
	SLOT_MACHINE_SPIN,
	SLOT_MACHINE_PAYOUT, // this may not be necessary
	SLOT_MACHINE_EXIT,
} slot_machine_state = SLOT_MACHINE_INIT;

bool bonus_active;
uint32_t outcome;

/*- Finance ------------------------------------------------------------------*/
#define SLOT_MACHINE_CREDITS_START 100

static int16_t credits;
static int bet = 1;
static int win;

#define BET_MIN (1)
#define BET_MAX (3)

static void bet_increase()
{
	if (bet < BET_MAX)
	{
		bet++;
		audio_out_beep(600, 200);
	}
}

static void bet_decrease()
{
	if (bet > BET_MIN)
	{
		bet--;
		audio_out_beep(400, 100);
	}
}

static void bet_take()
{
	credits -= bet;
}

/*- Reels --------------------------------------------------------------------*/
#define REEL_COUNT	(3)
#define REEL_BITS	(4)
#define REEL_SIZE	(1 << REEL_BITS)

enum Symbol
{
	SYM_NONE = 0,
	SYM_BAR1,
	SYM_BAR2,
	SYM_BAR3,
	SYM_CHERRY,
	SYM_SEVEN,
	SYM_BONUS,
	SYM_COUNT,
};

static const enum Symbol REEL[REEL_COUNT][REEL_SIZE] = 
{
	/*
	 *  The first reel is required to win. Therefore it is an ideal place to
	 *  put cherries close ot other symbols to lessen the tendency for cherries
	 *  to offset all losses by allowing them to be overriden by other wins.
	 */
	{SYM_NONE, SYM_BAR1, SYM_BAR2, SYM_BAR3, 
	 SYM_SEVEN, SYM_NONE, SYM_NONE, SYM_BAR1,
	 SYM_CHERRY, SYM_BAR2, SYM_NONE, SYM_NONE,
	 SYM_BAR3, SYM_SEVEN, SYM_BAR1, SYM_NONE},

	 /*
	  *  The second reel must be appropriately stingy to prevent excessive 
	  *  payouts for payscales which require only the first to symbols. 
	  */
	{SYM_BAR3, SYM_SEVEN, SYM_BAR2, SYM_BAR1,
	 SYM_NONE, SYM_NONE, SYM_BAR1, SYM_NONE,
	 SYM_CHERRY, SYM_NONE, SYM_BAR2, SYM_BAR3,
	 SYM_NONE, SYM_BAR1, SYM_NONE, SYM_NONE},

	/*
	 *	The third reel is all about the suspense. It's the "make or break" reel.
	 *  To that effect, it should always feel like you missed the payout by 
	 *  "that much". This can be accomplished by having a very spread out
	 *  and relatively rich set of symbols on the reel. It's also the best place
	 *  to put the "BONUS!" free spin symbol, as it helps to fill the reel and
	 *  enhance the feeling of having another go.
	 */
	{SYM_BONUS, SYM_NONE, SYM_BAR3, SYM_BAR2,
	 SYM_BAR1, SYM_NONE, SYM_SEVEN, SYM_CHERRY,
	 SYM_NONE, SYM_BAR1, SYM_BAR2, SYM_NONE,
	 SYM_SEVEN, SYM_BAR1, SYM_BAR2, SYM_NONE},
};

static int reel_position[REEL_COUNT];
static int target_position[REEL_COUNT];

static int reel_add_offset(size_t reel, int offset)
{
	int pos = (reel_position[reel] + offset) % REEL_SIZE;
	if (pos < 0)
	{
		pos += REEL_SIZE;
	}
	return pos;
}

static void reel_advance(size_t reel)
{
	reel_position[reel] = reel_add_offset(reel, -1);
}

static enum Symbol symbol_in_pos(size_t reel, int offset)
{
	return REEL[reel][reel_add_offset(reel, offset)];
}

static bool symbol_is_bar(enum Symbol sym)
{
	return (sym >= SYM_BAR1) && (sym <= SYM_BAR3);
}


/*- Payscales ----------------------------------------------------------------*/
enum Payout
{
	PAY_NONE = 0,
	PAY_CHERRY,
	PAY_BAR_ANY2,
	PAY_BAR_ANY3,
	PAY_BAR2_3,
	PAY_BAR3_3,
	PAY_SEVEN_3,
	PAY_CHERRY_3,
	PAY_COUNT,
};

static enum Payout last_payout;

struct Payscale
{
	char* description;
	int multiplier;
	enum Payout (*evaluation)();
	struct 
	{
		uint16_t freq;
		uint16_t duration;
	} beep;
	uint8_t led_duty[3];
};

static enum Payout eval_cherry()
{
	for (size_t reel = 0; reel < REEL_COUNT; reel++)
	{
		for (int offset = -1; offset < 2; offset++)
		{
			if (SYM_CHERRY == symbol_in_pos(reel, offset))
			{
				return PAY_CHERRY;
			}
		}
	}
	return PAY_NONE;
}

static enum Payout eval_bar_any2()
{
	if (symbol_is_bar(symbol_in_pos(0, 0))
	    && symbol_is_bar(symbol_in_pos(1, 0))
		&& !symbol_is_bar(symbol_in_pos(2, 0)))
	{
		return PAY_BAR_ANY2;
	}
	else
	{
		return PAY_NONE;
	}
}

static enum Payout eval_bar_any3()
{
	if (symbol_is_bar(symbol_in_pos(0, 0))
	    && symbol_is_bar(symbol_in_pos(1, 0))
		&& symbol_is_bar(symbol_in_pos(2, 0)))
	{
		return PAY_BAR_ANY3;
	}
	else
	{
		return PAY_NONE;
	}
}

static enum Payout eval_bar2_3()
{
	if ((SYM_BAR2 == (symbol_in_pos(0, 0)))
	    && (SYM_BAR2 == (symbol_in_pos(1, 0)))
		&& (SYM_BAR2 == symbol_in_pos(2, 0)))
	{
		return PAY_BAR2_3;
	}
	else
	{
		return PAY_NONE;
	}
}

static enum Payout eval_bar3_3()
{
	if ((SYM_BAR3 == (symbol_in_pos(0, 0)))
	    && (SYM_BAR3 == (symbol_in_pos(1, 0)))
		&& (SYM_BAR3 == symbol_in_pos(2, 0)))
	{
		return PAY_BAR3_3;
	}
	else
	{
		return PAY_NONE;
	}
}

static enum Payout eval_seven_3()
{
	if ((SYM_SEVEN == (symbol_in_pos(0, 0)))
	    && (SYM_SEVEN == (symbol_in_pos(1, 0)))
		&& (SYM_SEVEN == symbol_in_pos(2, 0)))
	{
		return PAY_SEVEN_3;
	}
	else
	{
		return PAY_NONE;
	}
}

static enum Payout eval_cherry_3()
{
	if ((SYM_CHERRY == (symbol_in_pos(0, 0)))
	    && (SYM_CHERRY == (symbol_in_pos(1, 0)))
		&& (SYM_CHERRY == symbol_in_pos(2, 0)))
	{
		return PAY_CHERRY_3;
	}
	else
	{
		return PAY_NONE;
	}
}

static bool eval_bonus()
{
	for (size_t reel = 0; reel < REEL_COUNT; reel++)
	{
		if (SYM_BONUS == symbol_in_pos(reel, 0))
		{
			return true;
		}
	}
	return false;
}

static const struct Payscale PAYSCALE[] =
{
	{"None", 0, NULL, {200, 50}, {0}},
	{"Cherry anywhere", 1, eval_cherry, {1500, 100}, {50, 0, 0}},
	{"Any 2 BAR", 10, eval_bar_any2, {1600, 200}, {50, 40, 0}},
	{"Any 3 BAR", 20, eval_bar_any3, {1650, 250}, {50, 40, 0}},
	{"Three BAR2", 500, eval_bar2_3, {1750, 600}, {50, 40, 0}},
	{"Three BAR3", 750, eval_bar3_3, {1900, 2000}, {100, 80, 0}},
	{"Three sevens", 1000, eval_seven_3, {2000, 3000}, {100, 100, 100}},
	{"Three cherries", 4000, eval_cherry_3, {2100, 4000}, {100, 0, 0}},
};

//#define FORCE_PAYOUT

static enum Payout payout_get()
{
#ifdef FORCE_PAYOUT
	int b = button_down_latches();
	switch(b)
	{
		case BUTTON_PRESSED(BADGE_BUTTON_DOWN, 0xff):
			return PAY_NONE;
		case BUTTON_PRESSED(BADGE_BUTTON_SW, 0xff):
			return PAY_CHERRY;
		case BUTTON_PRESSED(BADGE_BUTTON_LEFT, 0xff):
			return PAY_BAR_ANY2;
		case BUTTON_PRESSED(BADGE_BUTTON_LEFT, 0xff) | BUTTON_PRESSED(BADGE_BUTTON_UP, 0xff):
			return PAY_BAR_ANY3;
		case BUTTON_PRESSED(BADGE_BUTTON_RIGHT, 0xff):
			return PAY_BAR2_3;
		case BUTTON_PRESSED(BADGE_BUTTON_RIGHT, 0xff) | BUTTON_PRESSED(BADGE_BUTTON_UP, 0xff):
			return PAY_BAR3_3;
		case BUTTON_PRESSED(BADGE_BUTTON_RIGHT, 0xff) | BUTTON_PRESSED(BADGE_BUTTON_UP, 0xff) | BUTTON_PRESSED(BADGE_BUTTON_SW, 0xff):
			return PAY_SEVEN_3;	
		case BUTTON_PRESSED(BADGE_BUTTON_SW, 0xff) | BUTTON_PRESSED(BADGE_BUTTON_UP, 0xff):
			return PAY_CHERRY_3;
		default:
			break;
	}
#endif
	size_t i;
	enum Payout payout;
	for (i = PAY_COUNT - 1; i > 0; i--)
	{
		payout = PAYSCALE[i].evaluation();
		if (PAY_NONE != payout)
		{
			break;
		}
	}
	return payout;
}

/*- Rendering ----------------------------------------------------------------*/

#define REND_CHAR_WIDTH (8)
#define REND_PADDING (10)
#define REND_REEL_HEIGHT (80)
#define RENDER_REELS_OUTLINE_COLOR WHITE
#define REND_BAR_V2 1

static const char SYM_REPR_NONE_CMAP[1][3] =
{
	0
};

static const char SYM_REPR_NONE_PIX[32] =
{
	0
};

#if !REND_BAR_V2
static const char SYM_REPR_BAR1_PIX[64] = 
{
	0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55,
	0x00, 0x00, 0x00, 0x00,
	0x15, 0x55, 0x55, 0x54, 
	0x10, 0x54, 0x50, 0x54,
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x10, 0x54,
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x11, 0x14,
	0x15, 0x55, 0x55, 0x54,
	0x00, 0x00, 0x00, 0x00,
	0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55
};

static const char SYM_REPR_BAR1_CMAP[4][3] = 
{
	{0, 255, 127},
};

static const char SYM_REPR_BAR2_PIX[64] = 
{
	0x55, 0x55, 0x55, 0x55,
	0x00, 0x00, 0x00, 0x00, 
	0x10, 0x54, 0x50, 0x54,
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x10, 0x54,
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x11, 0x14,
	0x00, 0x00, 0x00, 0x00,
	0x10, 0x54, 0x50, 0x54,
	0x11, 0x10, 0x11, 0x14, 
	0x10, 0x51, 0x10, 0x54, 
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x11, 0x14,
	0x00, 0x00, 0x00, 0x00,
	0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55
};

static const char SYM_REPR_BAR2_CMAP[4][3] = 
{
	{255, 0, 0},
};

static const char SYM_REPR_BAR3_PIX[64] = 
{
	0x00, 0x00, 0x00, 0x00,
	0x11, 0x14, 0x51, 0x14,
	0x10, 0x51, 0x10, 0x54,
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x11, 0x14,
	0x00, 0x00, 0x00, 0x00,
	0x11, 0x14, 0x51, 0x14,
	0x10, 0x51, 0x10, 0x54,
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x11, 0x14,
	0x00, 0x00, 0x00, 0x00,
	0x11, 0x14, 0x51, 0x14,
	0x10, 0x51, 0x10, 0x54,
	0x11, 0x10, 0x11, 0x14,
	0x10, 0x51, 0x11, 0x14,
	0x00, 0x00, 0x00, 0x00
};

static const char SYM_REPR_BAR3_CMAP[4][3] = 
{
	{255, 127, 0},
};

#else

static const char SYM_REPR_BAR1_PIX[64] = 
{
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x55, 0x55, 0x55, 0x56,
	0x50, 0x54, 0x50, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x10, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x11, 0x16,
	0xaa, 0xaa, 0xaa, 0xaa,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};

static const char SYM_REPR_BAR1_CMAP[4][3] = 
{
	{255, 255, 255},
	{0, 127, 47},
	{0, 91, 31}
};

static const char SYM_REPR_BAR2_PIX[64] =
{
	0xff, 0xff, 0xff, 0xff,
	0x55, 0x55, 0x55, 0x56,
	0x50, 0x54, 0x50, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x10, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x11, 0x16,
	0xaa, 0xaa, 0xaa, 0xaa,
	0x50, 0x54, 0x50, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x10, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x11, 0x16,
	0xaa, 0xaa, 0xaa, 0xaa,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};

static const char SYM_REPR_BAR2_CMAP[4][3] = 
{
	{255, 255, 255},
	{255, 0, 0},
	{127, 0, 0},
};

static const char SYM_REPR_BAR3_PIX[64] = 
{
	0x50, 0x54, 0x50, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x10, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x11, 0x16,
	0xaa, 0xaa, 0xaa, 0xaa,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x10, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x11, 0x16,
	0xaa, 0xaa, 0xaa, 0xaa,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x10, 0x56,
	0x51, 0x10, 0x11, 0x16,
	0x50, 0x51, 0x11, 0x16,
	0xaa, 0xaa, 0xaa, 0xaa
};

static const char SYM_REPR_BAR3_CMAP[4][3] =
{
	{255, 255, 255},
	{221, 127, 0},
	{127, 63, 0},
};

#endif

static const char SYM_REPR_CHERRY_PIX[128] = 
{
	0x44, 0x44, 0x44, 0x44, 0x40, 0x14, 0x40, 0x14,
	0x44, 0x44, 0x44, 0x44, 0x40, 0x04, 0x01, 0x01,
	0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x10, 0x01,
	0x44, 0x44, 0x44, 0x40, 0x01, 0x00, 0x00, 0x14,
	0x44, 0x44, 0x44, 0x00, 0x10, 0x04, 0x11, 0x14,
	0x44, 0x44, 0x40, 0x01, 0x40, 0x14, 0x44, 0x44,
	0x44, 0x44, 0x00, 0x04, 0x40, 0x14, 0x44, 0x44,
	0x44, 0x22, 0x22, 0x14, 0x40, 0x14, 0x44, 0x44,
	0x42, 0x22, 0x22, 0x24, 0x32, 0x22, 0x44, 0x44,
	0x22, 0x22, 0x22, 0x22, 0x32, 0x22, 0x24, 0x44,
	0x22, 0x22, 0x22, 0x22, 0x32, 0x22, 0x22, 0x44,
	0x22, 0x22, 0x22, 0x22, 0x32, 0x22, 0x22, 0x44,
	0x22, 0x22, 0x22, 0x22, 0x32, 0x22, 0x22, 0x44,
	0x42, 0x22, 0x22, 0x23, 0x22, 0x22, 0x22, 0x44,
	0x44, 0x22, 0x22, 0x42, 0x22, 0x22, 0x24, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x22, 0x22, 0x44, 0x44
};

static const char SYM_REPR_CHERRY_CMAP[8][3] = 
{
	{0, 229, 0},
	{0, 155, 0},
	{255, 0, 0},
	{150, 0, 0},
};

static const char SYM_REPR_SEVEN_PIX[64] = 
{
	0x00, 0x0a, 0x80, 0x00,
	0x15, 0x40, 0x15, 0x54,
	0x15, 0x55, 0x55, 0x54,
	0x15, 0x55, 0x55, 0x54,
	0x14, 0x15, 0x50, 0x14, 
	0x14, 0x80, 0x05, 0x50,
	0x00, 0xaa, 0x15, 0x52,
	0xaa, 0xa8, 0x55, 0x4a,
	0xaa, 0xa1, 0x55, 0x2a,
	0xaa, 0x85, 0x54, 0xaa,
	0xaa, 0x15, 0x54, 0xaa,
	0xaa, 0x15, 0x54, 0xaa,
	0xa8, 0x55, 0x52, 0xaa,
	0xa8, 0x55, 0x52, 0xaa,
	0xa8, 0x55, 0x52, 0xaa,
	0xa8, 0x00, 0x02, 0xaa
};

static const char SYM_REPR_SEVEN_CMAP[4][3] = 
{
	{255, 255, 255},
	{255, 0, 0},
};

static const char SYM_REPR_BONUS_PIX[64] = 
{
	0xaa, 0xaa, 0xaa, 0xaa,
	0xaa, 0xa0, 0x0a, 0xaa,
	0xaa, 0xa0, 0x0a, 0xaa,
	0xaa, 0x90, 0x06, 0xaa,
	0xa5, 0x50, 0x05, 0x5a,
	0x95, 0x50, 0x05, 0x56,
	0x55, 0x50, 0x05, 0x55,
	0x55, 0x50, 0x05, 0x55,
	0x55, 0x50, 0x05, 0x55,
	0x55, 0x50, 0x05, 0x55,
	0x95, 0x50, 0x05, 0x56,
	0xa5, 0x55, 0x55, 0x5a,
	0xaa, 0x90, 0x06, 0xaa,
	0xaa, 0xa0, 0x0a, 0xaa,
	0xaa, 0xa0, 0x0a, 0xaa,
	0xaa, 0xaa, 0xaa, 0xaa
};

static const char SYM_REPR_BONUS_CMAP[4][3] = 
{
	{246, 255, 0},
	{74, 55, 207},
};

static const struct asset SYM_REPR[SYM_COUNT] =
{
	{0, PICTURE1BIT, 1, 16, 16, SYM_REPR_NONE_CMAP[0], SYM_REPR_NONE_PIX, NULL},
	{0, PICTURE2BIT, 1, 16, 16, SYM_REPR_BAR1_CMAP[0], SYM_REPR_BAR1_PIX, NULL},
	{0, PICTURE2BIT, 1, 16, 16, SYM_REPR_BAR2_CMAP[0], SYM_REPR_BAR2_PIX, NULL},
	{0, PICTURE2BIT, 1, 16, 16, SYM_REPR_BAR3_CMAP[0], SYM_REPR_BAR3_PIX, NULL},
	{0, PICTURE4BIT, 1, 16, 16, SYM_REPR_CHERRY_CMAP[0], SYM_REPR_CHERRY_PIX, NULL},
	{0, PICTURE2BIT, 1, 16, 16, SYM_REPR_SEVEN_CMAP[0], SYM_REPR_SEVEN_PIX, NULL},
	{0, PICTURE2BIT, 1, 16, 16, SYM_REPR_BONUS_CMAP[0], SYM_REPR_BONUS_PIX, NULL},
};

static void render_symbol(size_t reel, int offset)
{
	enum Symbol sym = symbol_in_pos(reel, offset);
	if (sym < (sizeof(SYM_REPR) / sizeof(SYM_REPR[0])))
	{
		FbMoveRelative(-8,-8);
		FbImage(&SYM_REPR[sym], 0);
	}
}

static void render_reels()
{
	size_t padding = REND_PADDING;
	size_t reel_width = (LCD_XSIZE - (REND_PADDING * (REEL_COUNT + 1)))/ REEL_COUNT;
	size_t reel_height = 80;
	int symbol_spacing = (reel_height - (2 * padding)) / 3;
	for (size_t reel = 0; reel < REEL_COUNT; reel++)
	{
		size_t left = padding * (reel + 1) + reel_width * reel;
		for (int offset = -2; offset < 3; offset++)
		{
			FbMove(left + reel_width / 2, padding + (reel_height / 2) + (symbol_spacing * offset));
			render_symbol(reel, offset);
		}
		FbColor(BLACK);
		FbMove(left,0);
		FbFilledRectangle(reel_width, padding);
		FbMove(left,padding + reel_height);
		FbFilledRectangle(reel_width, padding);
		/* Draw frame */
		FbColor(RENDER_REELS_OUTLINE_COLOR);
		FbMove(left, padding);
		FbRectangle(reel_width, reel_height);
	}
	FbColor(YELLOW);
	FbLine(padding, padding + reel_height / 2, 
		   LCD_XSIZE - padding-REEL_COUNT, padding + reel_height / 2);
}

static void render_instructions()
{
	FbColor(GREY16);
	FbMove(REND_PADDING, LCD_YSIZE - REND_PADDING - REND_CHAR_WIDTH);
	FbWriteString("<Exit  ^/v");
}

static void render_credits()
{
	char s[16] = {0};
	snprintf(s, sizeof(s), "Credit\n%-6d", credits);
	FbColor(WHITE);
	FbMove(REND_PADDING, REND_PADDING + REND_REEL_HEIGHT + REND_CHAR_WIDTH / 2);
	FbWriteString(s);
}

static void render_bet()
{
	char s[16] = {0};
	snprintf(s, sizeof(s), "Bet\n%2d", bet);
	FbColor(WHITE);
	FbMove(LCD_XSIZE / 2, 
		   REND_PADDING + REND_REEL_HEIGHT + REND_CHAR_WIDTH / 2);
	FbWriteString(s);
}

static void render_win()
{
	char s[16] = {0};
	snprintf(s, sizeof(s), "Win\n%3d", win);
	FbColor(WHITE);
	FbMove(LCD_XSIZE - REND_PADDING - REND_CHAR_WIDTH * 3, 
		   REND_PADDING + REND_REEL_HEIGHT + REND_CHAR_WIDTH / 2);
	FbWriteString(s);
}

static void render_bonus()
{
	const char s[] = "BONUS!";
	FbColor(YELLOW);
	FbBackgroundColor(PACKRGB(9, 7, 53));
	FbMove(42, LCD_YSIZE / 2);
	FbWriteLine(s);
	FbBackgroundColor(BLACK);
}

static void render()
{
	FbClear();
	render_reels();
	render_instructions();
	render_credits();
	render_bet();
	render_win();
	if (bonus_active)
	{
		render_bonus();
	}
	FbSwapBuffers();
}

/*- Runtime ------------------------------------------------------------------*/
static void slot_machine_init(void)
{
	FbInit();
	FbClear();
	slot_machine_state = SLOT_MACHINE_BET;
	size_t br = flash_kv_get_binary("casino/credits", &credits, sizeof(credits));
	if (br != sizeof(credits))
	{
		credits = SLOT_MACHINE_CREDITS_START;
	}
	random_insecure_bytes((void *) &outcome, sizeof(outcome));
	render();
}

static void spin()
{
	outcome = random_insecure_u32_congruence(outcome);
	for (size_t reel = 0; reel < REEL_COUNT; reel++)
	{
		target_position[reel] = (outcome >> (REEL_BITS * reel)) % REEL_SIZE;
	}
	slot_machine_state = SLOT_MACHINE_SPIN;	
}

static void pull_handle()
{
	if (!bonus_active)
	{
		bet_take();
	}
	spin();
}

static void led_rgb_off()
{
	led_pwm_disable(BADGE_LED_RGB_RED);
	led_pwm_disable(BADGE_LED_RGB_GREEN);
	led_pwm_disable(BADGE_LED_RGB_BLUE);
}

static void audio_play_jingle()
{
	char offset[4];
	random_insecure_bytes((void *) offset, sizeof(offset));
	audio_out_beep(1000 + offset[0], 1000 / 60 + 10);
	led_pwm_enable(BADGE_LED_RGB_RED, offset[1]/8);
	led_pwm_enable(BADGE_LED_RGB_GREEN, offset[2]/8);
	led_pwm_enable(BADGE_LED_RGB_BLUE, offset[3]/8);
}

static void slot_machine_bet()
{
	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
	{
		bet_increase();
		render();
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
	{
		bet_decrease();
		render();
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches))
	{
		slot_machine_state = SLOT_MACHINE_EXIT;
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
	{
		/* TODO implement pay scale screen */
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
	{
		pull_handle();
		render();
	}
}

static void slot_machine_spin()
{
	static size_t positions_spun = 0;
	size_t reel;
	for (reel = 0; reel < REEL_COUNT; reel++)
	{
		if ((reel_position[reel] != target_position[reel])
			|| (positions_spun < REEL_SIZE-1))
		{
			break;
		}
	}
	if (REEL_COUNT == reel)
	{
		slot_machine_state = SLOT_MACHINE_PAYOUT;
		positions_spun = 0;
	}
	else
	{
		positions_spun++;
		for(; reel < REEL_COUNT; reel++)
		{
			reel_advance(reel);
		}
	}
	
	audio_play_jingle();
	render();
}

static void slot_machine_payout()
{
	last_payout = payout_get();
	const struct Payscale *payscale = PAYSCALE+last_payout;
	win = payscale->multiplier * bet;
	credits += win;
	flash_kv_store_binary("casino/credits", &credits, sizeof(credits));
	
	slot_machine_state = SLOT_MACHINE_BET;
	
	audio_out_beep(payscale->beep.freq, payscale->beep.duration);
	for (size_t rgb = BADGE_LED_RGB_RED; rgb < 3; rgb++)
	{
		uint8_t duty = payscale->led_duty[rgb];
		if (duty)
		{
			led_pwm_enable(rgb, duty);
		}
		else
		{
			led_pwm_disable(rgb);
		}
	}

	if (eval_bonus())
	{
		/* free spin if on pay line */
		bonus_active = true;
		audio_out_beep(4000, 100);
		led_pwm_enable(BADGE_LED_RGB_RED, 5);
		led_pwm_enable(BADGE_LED_RGB_GREEN, 4);
		led_pwm_enable(BADGE_LED_RGB_BLUE, 40);		
	}
	else
	{
		bonus_active = false;
	}
	render();
}

static void slot_machine_exit()
{
	slot_machine_state = SLOT_MACHINE_INIT;
	led_rgb_off();
	returnToMenus();
}

void slot_machine_cb(void)
{
	switch (slot_machine_state) {
	case SLOT_MACHINE_INIT:
		slot_machine_init();
		break;
	case SLOT_MACHINE_BET:
		slot_machine_bet();
		break;
	case SLOT_MACHINE_SPIN:
		slot_machine_spin();
		break;
	case SLOT_MACHINE_PAYOUT:
		slot_machine_payout();
		break;
	case SLOT_MACHINE_EXIT:
		slot_machine_exit();
		break;
	default:
		break;
	}
}

