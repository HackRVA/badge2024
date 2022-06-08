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

/*- Finance ------------------------------------------------------------------*/
static int16_t credits = 100;
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
#define REEL_SIZE	(10)

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
	{SYM_NONE, SYM_BAR1, SYM_BAR2, SYM_BAR3, SYM_NONE, SYM_SEVEN, SYM_BAR1, SYM_NONE, SYM_CHERRY, SYM_BAR1},
	{SYM_BAR3, SYM_SEVEN, SYM_BAR2, SYM_BAR1, SYM_NONE, SYM_NONE, SYM_BAR1, SYM_NONE, SYM_CHERRY, SYM_BAR2},
	{SYM_BONUS, SYM_NONE, SYM_BAR3, SYM_BAR2, SYM_BAR1, SYM_NONE, SYM_NONE, SYM_SEVEN, SYM_NONE, SYM_BAR1},
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
	reel_position[reel] = reel_add_offset(reel, 1);
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
	PAY_BAR2_2,
	PAY_BAR2_3,
	PAY_BAR3_2,
	PAY_BAR3_3,
	PAY_SEVEN_2,
	PAY_SEVEN_3,
	PAY_COUNT,
};

static enum Payout last_payout;
static const enum Payout JACKPOT_THRESHOLD = PAY_BAR3_2; // use to determine jackpot celebration

struct Payscale
{
	char* description;
	int multiplier;
	enum Payout (*evaluation)();
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

static enum Payout eval_bar2_2()
{
	if ((SYM_BAR2 == (symbol_in_pos(0, 0)))
	    && (SYM_BAR2 == (symbol_in_pos(1, 0)))
		&& (SYM_BAR2 != symbol_in_pos(2, 0)))
	{
		return PAY_BAR2_2;
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

static enum Payout eval_bar3_2()
{
	if ((SYM_BAR3 == (symbol_in_pos(0, 0)))
	    && (SYM_BAR3 == (symbol_in_pos(1, 0)))
		&& (SYM_BAR3 != symbol_in_pos(2, 0)))
	{
		return PAY_BAR3_2;
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

static enum Payout eval_seven_2()
{
	if ((SYM_SEVEN == (symbol_in_pos(0, 0)))
	    && (SYM_SEVEN == (symbol_in_pos(1, 0)))
		&& (SYM_SEVEN != symbol_in_pos(2, 0)))
	{
		return PAY_SEVEN_2;
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
	{"None", 0, NULL},
	{"Cherry anywhere", 1, eval_cherry},
	{"Any 2 BAR", 2, eval_bar_any2},
	{"Any 3 BAR", 3, eval_bar_any3},
	{"Two BAR2", 5, eval_bar2_2},
	{"Three BAR2", 10, eval_bar2_3},
	{"Two BAR3", 20, eval_bar3_2},
	{"Three BAR3", 35, eval_bar3_3},
	{"Two sevens", 50, eval_seven_2},
	{"Three sevens", 100, eval_seven_3}
};

static enum Payout payout_get()
{
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
#define RENDER_REELS_OUTLINE_COLOR WHITE

static void render_symbol(size_t reel, int offset)
{
	static const char repr[SYM_COUNT] = {' ','1','2','3','C','7','B'};
	FbMoveRelative(-REND_CHAR_WIDTH/2,-REND_CHAR_WIDTH/2);
	FbCharacter(repr[symbol_in_pos(reel, offset)]);
}

static void render_reels()
{
	size_t padding = REND_PADDING;
	size_t reel_width = (LCD_XSIZE - (REND_PADDING * (REEL_COUNT + 1)))/ REEL_COUNT;
	size_t reel_height = 80;
	int symbol_spacing = (reel_height - (2 * padding)) / 3;
	FbColor(RENDER_REELS_OUTLINE_COLOR);
	for (size_t reel = 0; reel < REEL_COUNT; reel++)
	{
		size_t left = padding * (reel + 1) + reel_width * reel;
		FbMove(left, padding);
		FbRectangle(reel_width, reel_height);
		for (int offset = -1; offset < 2; offset++)
		{
			FbMove(left + reel_width / 2, padding + (reel_height / 2) + (symbol_spacing * offset));
			render_symbol(reel, offset);
		}
	}
	FbColor(YELLOW);
	FbLine(padding, padding + reel_height / 2, 
		   LCD_XSIZE - padding-REEL_COUNT, padding + reel_height / 2);
}

static void render_credits()
{
	char s[16] = {0};
	snprintf(s, sizeof(s), "Credit\n%-6d", credits);
	FbColor(WHITE);
	FbMove(REND_PADDING, LCD_YSIZE - REND_PADDING - REND_CHAR_WIDTH * 2);
	FbWriteString(s);
}

static void render_bet()
{
	char s[16] = {0};
	snprintf(s, sizeof(s), "Bet\n%2d", bet);
	FbColor(WHITE);
	FbMove(LCD_XSIZE / 2, 
		   LCD_YSIZE - REND_PADDING - REND_CHAR_WIDTH * 2);
	FbWriteString(s);
}

static void render_win()
{
	char s[16] = {0};
	snprintf(s, sizeof(s), "Win\n%3d", win);
	FbColor(WHITE);
	FbMove(LCD_XSIZE - REND_PADDING - REND_CHAR_WIDTH * 3, 
		   LCD_YSIZE - REND_PADDING - REND_CHAR_WIDTH * 2);
	FbWriteString(s);
}

static void render_bonus()
{
	const char s[] = "   B O N U S";
	FbColor(YELLOW);
	FbMove(REND_PADDING, LCD_YSIZE / 2);
	FbWriteLine(s);
}

static void render()
{
	FbClear(); // FIXME only render what has changed
	render_reels();
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
}

static void spin()
{
	size_t outcome;
	random_insecure_bytes((void *) &outcome, sizeof(outcome));
	for (size_t reel = 0; reel < REEL_COUNT; reel++)
	{
		size_t divisor = REEL_SIZE * (reel + 1);
		target_position[reel] = (outcome % divisor) / (divisor / REEL_SIZE);
	}
	slot_machine_state = SLOT_MACHINE_SPIN;
}

static void pull_handle()
{
	bonus_active = false;
	bet_take();
	spin();
}

static void audio_play_jingle()
{
	char offset;
	random_insecure_bytes((void *) &offset, sizeof(offset));
	audio_out_beep(1000 + offset, 1000 / 60 + 10);
}

static void slot_machine_bet()
{
	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
	{
		bet_increase();
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
	{
		bet_decrease();
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches))
	{
		slot_machine_state = SLOT_MACHINE_EXIT;
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
	{
		/* TODO implement pay scale screen */
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
	{
		pull_handle();
	}
	render();
}

static void slot_machine_spin()
{
	size_t reel;
	for (reel = 0; reel < REEL_COUNT; reel++)
	{
		if (reel_position[reel] != target_position[reel])
		{
			break;
		}
	}
	if (REEL_COUNT == reel)
	{
		slot_machine_state = SLOT_MACHINE_PAYOUT;
	}
	else
	{
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
	win = last_payout * bet;
	credits += win;
	
	slot_machine_state = SLOT_MACHINE_BET;
	if (last_payout >= JACKPOT_THRESHOLD)
	{
		/* TODO add jackpot celebration; consider using LED */
		/* wait for any button */
	}
	
	if (PAY_CHERRY == last_payout)
	{
		audio_out_beep(1500,100);
	}
	
	else if (last_payout != PAY_NONE)
	{
		int multiplier = PAYSCALE[last_payout].multiplier;
		audio_out_beep(1600 + 10 * multiplier,100 + 50 * multiplier);
	}
	else
	{
		audio_out_beep(200,50);
	}
	

	if (eval_bonus())
	{
		/* free spin if on pay line */
		bonus_active = true;
		audio_out_beep(4000, 100);
		spin(); /* TODO delay for payout beep */
	}
	render();
}

static void slot_machine_exit()
{
	slot_machine_state = SLOT_MACHINE_INIT;
	returnToMenus();
}

/* You will need to rename myprogram_cb() something else. */
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

