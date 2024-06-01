/*!
 *  @file   rover_adventure.c
 *  @author Peter Maxwell Warasila
 *  @date   May 31, 2024
 *
 *  @brief  2024 Badge Rover Adventure App Implementation
 *
 *------------------------------------------------------------------------------
 *
 */

/* C std lib */
#include <stdio.h>
#include <stdint.h>

/* Badge system */
#include <colors.h>
#include <menu.h>
#include <dynmenu.h>
#include <utils.h>
#include <framebuffer.h>
#include <key_value_storage.h>

/* Badge hardware */
#include <audio.h>
#include <button.h>
#include <mic_pdm.h>

#include "rover_adventure.h"

/*- Private Macro ------------------------------------------------------------*/
#define KVS_KEY "radvtodo"

#define PURPLE (PACKRGB(15,2,31))

/*- Private Types ------------------------------------------------------------*/
/*---- Rover TODO items ------------------------------------------------------*/
enum rover_adventure_state {
	ROVER_ADVENTURE_INIT,
	ROVER_ADVENTURE_MENU,
	ROVER_ADVENTURE_EXIT,
	ROVER_ADVENTURE_AUDIO,
};

enum rover_todo {
	TODO_AUDIO_VLOUD = 0,
	TODO_AUDIO_LOUD,
	TODO_AUDIO_QUIET,
	TODO_AUDIO_VQUIET,

	TODO_AUDIO_START = TODO_AUDIO_VLOUD,
	TODO_AUDIO_END = TODO_AUDIO_VQUIET,

	TODO_COUNT
};

/*- Private Variables --------------------------------------------------------*/
enum rover_adventure_state m_radv_state = ROVER_ADVENTURE_INIT;

static struct dynmenu m_radv_menu;
static struct dynmenu_item m_radv_menu_items[100]; // FIXME idk n yet -PMW

/*----- Audio ----------------------------------------------------------------*/
static unsigned mic_index_wrapped;
static unsigned mic_index;
#define PEAK 50
#define AVG 20
static audio_sample_t mic_peaks[128] = {
	PEAK, PEAK, PEAK, PEAK, 
	PEAK, PEAK, PEAK, PEAK,
	PEAK, PEAK, PEAK, PEAK,
	PEAK, PEAK, PEAK, PEAK,
	PEAK, PEAK, PEAK, PEAK, 
	PEAK, PEAK, PEAK, PEAK,
	PEAK, PEAK, PEAK, PEAK,
	PEAK, PEAK, PEAK, PEAK,
};
static audio_sample_t mic_averages[128] = {
	AVG, AVG, AVG, AVG,
	AVG, AVG, AVG, AVG,
	AVG, AVG, AVG, AVG,
	AVG, AVG, AVG, AVG,
	AVG, AVG, AVG, AVG,
	AVG, AVG, AVG, AVG,
	AVG, AVG, AVG, AVG,
	AVG, AVG, AVG, AVG,
};
#undef AVG
#undef PEAK
static int8_t m_radv_peak_dBFS;
static int8_t m_radv_avg_dBFS;

static void radv_mic_cb(const audio_sample_t *samples, size_t len)
{
	mic_peaks[mic_index] = audio_peak(samples, len);
	mic_averages[mic_index] = audio_rms(samples, len);
	mic_index++;
	if (mic_index >= ARRAY_SIZE(mic_averages)) {
		mic_index_wrapped++;
		mic_index = 0;
	}
}

#define MIC_EVAL_ARG_DBMASK (0xFF)
#define MIC_EVAL_ARG_GT     (0x100)
#define MIC_EVAL_ARG_LT     (0)

static int radv_mic_eval_vloud(__attribute__((unused)) void *arg)
{
	return m_radv_avg_dBFS > -6;
}

static int radv_mic_eval_loud(__attribute__((unused)) void *arg)
{
	return (m_radv_avg_dBFS > -20) && (m_radv_avg_dBFS < -10);
}

static int radv_mic_eval_quiet(__attribute__((unused)) void *arg)
{
	return (m_radv_avg_dBFS > -50) && (m_radv_avg_dBFS < -40);
}

static int radv_mic_eval_vquiet(__attribute__((unused)) void *arg)
{
	return m_radv_avg_dBFS < -60;
}

static int radv_mic_eval_gt(void *argument)
{
	int8_t ref_dBFS = (intptr_t) argument;
	return m_radv_avg_dBFS > ref_dBFS;
}

static int radv_mic_eval_lt(void *argument)
{
	int8_t ref_dBFS = (intptr_t) argument;
	return m_radv_avg_dBFS < ref_dBFS;
}

static uint16_t radb_color_from_dBFS(int8_t dB)
{
	if (dB > -6)
		return RED;
	else if (dB > -18)
		return YELLOW;
	else if (dB > -44)
		return GREEN;
	else
		return BLUE;
}


/*----- Rover TODO List ------------------------------------------------------*/
static uint32_t m_radv_todo;

static struct radv_todo {
	const char *const str;
	int (*const eval)(void *arg);
	void *const arg;
} TODO_LIST[TODO_COUNT] = {
	{"v. loud", radv_mic_eval_vloud, NULL},
	{"loud", radv_mic_eval_loud, NULL},
	{"quiet", radv_mic_eval_quiet, NULL},
	{"v. quiet", radv_mic_eval_vquiet, NULL},
}; 

/*----- Helpers --------------------------------------------------------------*/
static void radv_todos(enum rover_todo first, enum rover_todo last)
{
	bool acheived = false;
	for (unsigned i = first; i <= last; i++) {
		struct radv_todo *t = &TODO_LIST[i];
		uint32_t m = 1 << i;
		/* Evaluate */
		if ((0 == (m_radv_todo & m))
		    && t->eval(t->arg)) {
			m_radv_todo |= m;
			acheived = true;
		}

		/* Draw */
		char s[10];
		(void) snprintf(s, sizeof(s), "+%s\n", t->str);
		if (m_radv_todo & m) {
			FbColor(GREEN);
		} else {
			FbColor(RED);
		}
		FbWriteString(s);
	}

	if (acheived) {
		/* Write back if anything was crossed off list. */
		flash_kv_store_binary(KVS_KEY, (void *) &m_radv_todo, 
				      sizeof(m_radv_todo));
	}
}

/* draw and check B button to go back to menu */
static void radv_b_for_back(int down_latches)
{
	FbMove(8,LCD_YSIZE - 16);
	FbColor(GREY8);
	FbWriteLine("(B) Back");
	if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		m_radv_state = ROVER_ADVENTURE_MENU;
	}
}

/*----- App States -----------------------------------------------------------*/
static void radv_init(void)
{
	/* Load achievements. */
	flash_kv_get_binary(KVS_KEY, (void *) &m_radv_todo, sizeof(m_radv_todo));

	/* Add mic calback. */
	mic_index = 0;
	mic_index_wrapped = 0;
	m_radv_peak_dBFS = -30;
	m_radv_avg_dBFS = -30;
	mic_add_cb(radv_mic_cb);

	/* Setup top level menu. */
	if (m_radv_menu.max_items == 0) {
		dynmenu_init(&m_radv_menu, m_radv_menu_items,
			     ARRAY_SIZE(m_radv_menu_items));
		dynmenu_clear(&m_radv_menu);
		dynmenu_set_colors(&m_radv_menu, PURPLE, GREEN);
		dynmenu_set_title(&m_radv_menu, "ROVER'S BIG", "ADVENTURE", "");
		dynmenu_add_item(&m_radv_menu, "AUDIO", ROVER_ADVENTURE_AUDIO, -1);
		dynmenu_add_item(&m_radv_menu, "EXIT", ROVER_ADVENTURE_EXIT, -1);
	}
	
	/* Jump to menu. */
	m_radv_state = ROVER_ADVENTURE_MENU;
}

static void radv_exit(void)
{
	/* Store achievements again just in case. */
	flash_kv_store_binary(KVS_KEY, (void *) &m_radv_todo, sizeof(m_radv_todo));

	/* Remove mic callback. */
	mic_remove_cb(radv_mic_cb);

	/* Reset for next. */
	m_radv_state = ROVER_ADVENTURE_INIT;

	returnToMenus();
}

static void radv_menu(void)
{
	dynmenu_draw(&m_radv_menu); // Force draw always bc screensavers suck -PMW
        FbPushBuffer();
	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		dynmenu_change_current_selection(&m_radv_menu, 1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		dynmenu_change_current_selection(&m_radv_menu, -1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		m_radv_state = m_radv_menu.item[m_radv_menu.current_item].next_state;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		m_radv_state = ROVER_ADVENTURE_EXIT;
	}
}

static void radv_audio(void)
{
	/* Wait for all samples to be filled. */
	if (1 < mic_index_wrapped) {
		/* Evaluate audio. */
		m_radv_peak_dBFS = audio_dBFS(audio_peak(mic_peaks,
							 ARRAY_SIZE(mic_peaks)));
		m_radv_avg_dBFS = audio_dBFS(audio_rms(mic_averages,
						       ARRAY_SIZE(mic_averages)));
	}

	/* Clear the screen. */
	FbBackgroundColor(BLACK);
	FbClear();

	/* Write the banner */ 
	FbMove(8,8);
	FbColor(WHITE);
	FbWriteString("AUDIO\n\nBring me\nsomewhere...\n\n");
	
	/* Draw and evaluate todos. */
	radv_todos(TODO_AUDIO_START, TODO_AUDIO_END);

	/* Draw audio bar. */
	unsigned i = mic_index + ARRAY_SIZE(mic_peaks) - 1;
	i %= ARRAY_SIZE(mic_peaks);

	int8_t peak_dBFS = audio_dBFS(mic_peaks[i]);
	FbColor(radb_color_from_dBFS(peak_dBFS));
	int peak_height = (80 + peak_dBFS) * 1;
	if (peak_height < 1) peak_height = 1;
	FbMove(102, LCD_YSIZE - (8 + peak_height));
	FbRectangle(16, 1);

	int8_t avg_dBFS = audio_dBFS(mic_averages[i]);
	FbColor(radb_color_from_dBFS(avg_dBFS));
	int avg_height = (80 + avg_dBFS) * 1;
	if (avg_height < 1) avg_height = 1;
	FbMove(102, LCD_YSIZE - (8 + avg_height));
	FbFilledRectangle(16, avg_height);

	radv_b_for_back(button_down_latches());

	FbPushBuffer();
}

void rover_adventure_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (m_radv_state) {
	case ROVER_ADVENTURE_INIT:
		radv_init();
		break;
	case ROVER_ADVENTURE_MENU:
		radv_menu();
		break;
	case ROVER_ADVENTURE_EXIT:
		radv_exit();
		break;
	case ROVER_ADVENTURE_AUDIO:
		radv_audio();
		break;
	default:
		/* Always return to know good state through INIT */
		m_radv_state = ROVER_ADVENTURE_INIT;
		break;
	}
}


