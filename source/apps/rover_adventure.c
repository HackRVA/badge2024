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
#include <stdlib.h>
#include <string.h>

/* Badge system */
#include <badge.h>
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
#include <color_sensor.h>
#include <led_pwm.h>
#include <delay.h>
#include <analog.h>

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
	ROVER_ADVENTURE_COLOR,
	ROVER_ADVENTURE_CONDUCTIVITY,
	ROVER_ADVENTURE_MAGNOMETER,
	ROVER_ADVENTURE_TEMPERATURE,
};

enum rover_todo {
	TODO_AUDIO_VLOUD = 0,
	TODO_AUDIO_LOUD,
	TODO_AUDIO_QUIET,
	TODO_AUDIO_VQUIET,

	TODO_AUDIO_START = TODO_AUDIO_VLOUD,
	TODO_AUDIO_END = TODO_AUDIO_VQUIET,

	TODO_COLOR_BRIGHT,
	TODO_COLOR_DIM,
	TODO_COLOR_OUTSIDE,

	TODO_COLOR_START = TODO_COLOR_BRIGHT,
	TODO_COLOR_END = TODO_COLOR_OUTSIDE,

	TODO_CONDUCTIVITY_SHORT,
	TODO_CONDUCTIVITY_MID,
	TODO_CONDUCTIVITY_LOW,
	TODO_CONDUCTIVITY_LOW_BATT,

	TODO_CONDUCTIVITY_START = TODO_CONDUCTIVITY_SHORT,
	TODO_CONDUCTIVITY_END = TODO_CONDUCTIVITY_LOW_BATT,

	TODO_MAGNOMETER_NORTH,
	TODO_MAGNOMETER_SOUTH,
	TODO_MAGNOMETER_ALTERNATING,

	TODO_MAGNOMETER_START = TODO_MAGNOMETER_NORTH,
	TODO_MAGNOMETER_END = TODO_MAGNOMETER_ALTERNATING,

	TODO_TEMPERATURE_HOT,
	TODO_TEMPERATURE_WARM,
	TODO_TEMPERATURE_COOL,
	TODO_TEMPERATURE_COLD,

	TODO_TEMPERATURE_START = TODO_TEMPERATURE_HOT,
	TODO_TEMPERATURE_END = TODO_TEMPERATURE_COLD,

	TODO_COUNT
};

/*- Private Variables --------------------------------------------------------*/
enum rover_adventure_state m_radv_state = ROVER_ADVENTURE_INIT;

static struct dynmenu m_radv_menu;
static struct dynmenu_item m_radv_menu_items[100]; // FIXME idk n yet -PMW

/*----- Audio ----------------------------------------------------------------*/
static volatile unsigned mic_index_wrapped;
static volatile unsigned mic_index;
static audio_sample_t mic_peaks[128];
static audio_sample_t mic_averages[128];
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
	return (m_radv_avg_dBFS > -80) && (m_radv_avg_dBFS < -50);
}

static int radv_mic_eval_vquiet(__attribute__((unused)) void *arg)
{
	return m_radv_avg_dBFS <= -80;
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
	else if (dB > -48)
		return GREEN;
	else if (dB > -72)
		return BLUE;
	else
		return PURPLE;
}

/*----- Color sensor ---------------------------------------------------------*/
static struct color_sample m_radv_color_sample;

static void radv_color_sample(void)
{
	(void) color_sensor_get_sample(&m_radv_color_sample);
//	printf("\nsampled color:");
//	for (int i = 0; i < COLOR_SAMPLE_INDEX_COUNT; i++) {
//		printf(" %d", m_radv_color_sample.rgbwi[i]);
//	}
}

static int radv_color_senor_bright_eval(__attribute__((unused)) void *argument)
{
	static int bright_count = 0;
	if ((m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_RED] > 1000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_GREEN] > 2000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_BLUE] > 1000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_WHITE] > 8000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_IR] < 1000)) {
		bright_count++;
	} else {
		bright_count = 0;
	}

	return bright_count > 100;
}

static int radv_color_senor_dim_eval(__attribute__((unused)) void *argument)
{
	/* Dim but NOT dark. -PMW */
	static int dim_count = 0;
	if ((m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_RED] > 10)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_RED] < 50)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_GREEN] > 20)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_GREEN] < 100)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_BLUE] > 10)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_BLUE] < 50)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_WHITE] < 200)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_WHITE] > 40)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_IR] < 100)) {
		dim_count++;
	} else {
		dim_count = 0;
	}

	return dim_count > 100;
}

static int radv_color_senor_outside_eval(__attribute__((unused)) void *argument)
{
	static int outside_count = 0;
	if ((m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_RED] > 10000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_GREEN] > 20000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_BLUE] > 10000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_WHITE] > 30000)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_WHITE] < UINT16_MAX)
	     && (m_radv_color_sample.rgbwi[COLOR_SAMPLE_INDEX_IR] > 10000)) {
		outside_count++;
	} else {
		outside_count = 0;
	}

	return outside_count > 100;
}

/*----- Conductivity ---------------------------------------------------------*/
float m_radv_resistance = 1e6f;

struct radv_conductivity_spec {
	float lbound;
	float ubound;
};

static const struct radv_conductivity_spec radv_conductivity_short = {
	.lbound = 0.0f,
	.ubound = 20.0f
};

static const struct radv_conductivity_spec radv_conductivity_mid = {
	.lbound = 20.0f,
	.ubound = 2e4f,
};

static const struct radv_conductivity_spec radv_conductivity_low = {
	.lbound = 2e4f,
	.ubound = 2e5f,
};

static int radv_conductivity_eval(void *argument)
{
	const struct radv_conductivity_spec *spec = argument;
	if ((m_radv_resistance > spec->lbound)
	    && (m_radv_resistance < spec->ubound)) {
		return 1;
	} else {
		return 0;
	}
}

static int radv_conductivity_low_batt_eval(__attribute__((unused)) void *arg)
{
	uint32_t mV = analog_get_batt_mV();
	return (mV > 1000) && (mV < 2000);
}

static void radv_conductivity_measure(void)
{
	uint32_t mV = analog_get_chan_mV(ANALOG_CHAN_CONDUCTIVITY);
	m_radv_resistance = analog_calc_resistance_ohms(mV);
}

/*----- Magnometer -----------------------------------------------------------*/
unsigned m_magnet_index = 0;
unsigned m_magnet_wrapped = 0;
int8_t m_magnet_samples[30] = {0};

static int radv_magnometer_north_eval(__attribute__((unused)) void *arg)
{
	if (m_magnet_wrapped == 0) {
		return 0;
	}

	int8_t min = INT8_MAX;
	for (unsigned i = 0; i < ARRAY_SIZE(m_magnet_samples); i++) {
		int32_t sample = m_magnet_samples[i];
		min = MIN(sample, min);
	}

	return min > 80;
}

static int radv_magnometer_south_eval(__attribute__((unused)) void *arg)
{
	if (m_magnet_wrapped == 0) {
		return 0;
	}

	int8_t max = INT8_MIN;
	for (unsigned i = 0; i < ARRAY_SIZE(m_magnet_samples); i++) {
		int32_t sample = m_magnet_samples[i];
		max = MAX(sample, max);
	}

	return max < -80;
}

static int calc_sign(int n)
{
	return n > 0 ? 1 : n < 0 ? -1 : 0;
}

static int zero_crossed(int n, int prev_sign)
{
	int sign = calc_sign(n);
	if (((sign != 0) && (0 == prev_sign))
	    || ((sign > 0) && (prev_sign < 0))
	    || ((sign < 0) && (prev_sign > 0))) {
		return sign;
	} else {
		return 0;
	}
}

static int radv_magnometer_alternating_eval(__attribute__((unused)) void *arg)
{
	unsigned start = m_magnet_index + ARRAY_SIZE(m_magnet_samples);
	start %= ARRAY_SIZE(m_magnet_samples);

	int8_t sign_change_count = 0;
	int8_t local_extreme = m_magnet_samples[start];
	int8_t sign = calc_sign(local_extreme);

	for (unsigned u = 0; u < ARRAY_SIZE(m_magnet_samples); u++) {
		unsigned i = (start + u) % ARRAY_SIZE(m_magnet_samples);
		int8_t mT = m_magnet_samples[i];
		
		/* Update local extreme. */
		if (((sign > 0) && (mT > local_extreme))
		    || ((sign < 0) && (mT < local_extreme))) {
			local_extreme = mT;
		}
		
		/* Check for zero crossing. */
		if (zero_crossed(mT, sign)) {
			/* The local extreme must be at least 10 mT in magnitude. */
			if (abs(local_extreme) >= 10) {
				sign_change_count++;
			} else {
				/* Only count consecutive eligible excursions. */
				sign_change_count = 0;
			}

			local_extreme = mT;
			sign = calc_sign(mT);
		}
	}

	/* If we have at least MANY sign changes, achieved. */
	return sign_change_count >= 11;
}

static void radv_magnometer_measure(void)
{
	uint32_t mV = analog_get_chan_mV(ANALOG_CHAN_HALL_EFFECT);
	int8_t mT = analog_calc_hall_effect_mT(mV);

	m_magnet_samples[m_magnet_index++] = mT;
	if (m_magnet_index >= ARRAY_SIZE(m_magnet_samples)) {
		m_magnet_index = 0;
		m_magnet_wrapped = 1;
	}
}	

/*----- Temperature ----------------------------------------------------------*/
struct radv_temperature_spec {
	int8_t lbound;
	int8_t ubound;
};

static const struct radv_temperature_spec radv_temperature_hot = {
	.lbound = 55,
	.ubound = INT8_MAX,
};

static const struct radv_temperature_spec radv_temperature_warm = {
	.lbound = 35, // should be attainable with skin/body temp
	.ubound = 45,
};

static const struct radv_temperature_spec radv_temperature_cool = {
	.lbound = 12,
	.ubound = 22, // This is ~room temp but still needs active cooling to get
};

static const struct radv_temperature_spec radv_temperature_cold = {
	.lbound = INT8_MIN,
	.ubound = 10, // Be careful if using ice water!
};

static int radv_temperature_eval(void *arg)
{
	uint32_t mV = analog_get_chan_mV(ANALOG_CHAN_THERMISTOR);
	int8_t tC = analog_calc_thermistor_temp_C(mV);

	const struct radv_temperature_spec *spec = arg;
	if ((tC > spec->lbound) && (tC < spec->ubound)) {
		return 1;
	} else {
		return 0;
	}
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

	{"bright", radv_color_senor_bright_eval, NULL},
	{"dim\n", radv_color_senor_dim_eval, NULL},
	{"go\n outside", radv_color_senor_outside_eval, NULL},

	{"conductive", radv_conductivity_eval, (void *) &radv_conductivity_short},
	{"resistive", radv_conductivity_eval, (void *) &radv_conductivity_mid},
	{"almost\n insulative\n", radv_conductivity_eval, (void *) &radv_conductivity_low},
	{"operate\n on low\n battery", radv_conductivity_low_batt_eval, NULL},

	{"north pole", radv_magnometer_north_eval, NULL},
	{"south pole\n", radv_magnometer_south_eval, NULL},
	{"alternating\n poles", radv_magnometer_alternating_eval, NULL},

	{"hot", radv_temperature_eval, (void *) &radv_temperature_hot},
	{"warm", radv_temperature_eval, (void *) &radv_temperature_warm},
	{"cool", radv_temperature_eval, (void *) &radv_temperature_cool},
	{"cold", radv_temperature_eval, (void *) &radv_temperature_cold},
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
		char s[32];
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
	memset(mic_peaks, 0, sizeof(mic_peaks));
	memset(mic_averages, 0, sizeof(mic_averages));
	m_radv_peak_dBFS = -30;
	m_radv_avg_dBFS = -30;
	mic_add_cb(radv_mic_cb);

	/* Clear previous color sample. */
	memset(&m_radv_color_sample, 0, sizeof(m_radv_color_sample));

	/* Enable magnometer and clear previous magnetometer samples. */
	analog_set_sensor_power(ANALOG_SENSOR_POWER_ENABLED);

	m_magnet_index = 0;
	m_magnet_wrapped = 0;
	memset(m_magnet_samples, 0, sizeof(m_magnet_samples));

	/* Setup top level menu. */
	if (m_radv_menu.max_items == 0) {
		dynmenu_init(&m_radv_menu, m_radv_menu_items,
			     ARRAY_SIZE(m_radv_menu_items));
		dynmenu_clear(&m_radv_menu);
		dynmenu_set_colors(&m_radv_menu, PURPLE, GREEN);
		dynmenu_set_title(&m_radv_menu, "ROVER'S BIG", "ADVENTURE", "");
		dynmenu_add_item(&m_radv_menu, "AUDIO", ROVER_ADVENTURE_AUDIO, -1);
		dynmenu_add_item(&m_radv_menu, "LIGHT", ROVER_ADVENTURE_COLOR, -1);
		dynmenu_add_item(&m_radv_menu, "ELECTRONICS", ROVER_ADVENTURE_CONDUCTIVITY, -1);
		dynmenu_add_item(&m_radv_menu, "MAGNETICS", ROVER_ADVENTURE_MAGNOMETER, -1);
		dynmenu_add_item(&m_radv_menu, "TEMPERATURE", ROVER_ADVENTURE_TEMPERATURE, -1);
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

	/* Turn off any LEDs we have set. */
	led_pwm_disable(BADGE_LED_RGB_RED);
	led_pwm_disable(BADGE_LED_RGB_GREEN);
	led_pwm_disable(BADGE_LED_RGB_BLUE);

	/* Turn off magnometer. */
	analog_set_sensor_power(ANALOG_SENSOR_POWER_DISABLED);

	/* Reset for next. */
	m_radv_state = ROVER_ADVENTURE_INIT;

	returnToMenus();
}

static void radv_menu(void)
{
	if (m_radv_todo == ((1 << TODO_COUNT) - 1)) {
		led_pwm_disable(BADGE_LED_RGB_RED);
		led_pwm_enable(BADGE_LED_RGB_GREEN, 100);
		led_pwm_disable(BADGE_LED_RGB_BLUE);
	} else {
		led_pwm_enable(BADGE_LED_RGB_RED, 5);
		led_pwm_disable(BADGE_LED_RGB_GREEN);
		led_pwm_enable(BADGE_LED_RGB_BLUE, 50);
	}

	dynmenu_draw(&m_radv_menu); // Force draw always bc screensavers suck -PMW
        FbSwapBuffers();
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
	FbWriteString("AUDIO\n\n");
	FbColor(PURPLE);
	FbWriteString("Bring me\nsomewhere...\n\n");
	
	/* Draw and evaluate todos. */
	radv_todos(TODO_AUDIO_START, TODO_AUDIO_END);

	/* Draw audio bar. */
	unsigned i = mic_index + ARRAY_SIZE(mic_peaks) - 1;
	i %= ARRAY_SIZE(mic_peaks);

	static int8_t prev_peak_dBFS = INT8_MIN;
	int8_t peak_dBFS = audio_dBFS(mic_peaks[i]);
	if (prev_peak_dBFS > peak_dBFS) {
		peak_dBFS = --prev_peak_dBFS;
	} else {
		prev_peak_dBFS = peak_dBFS;
	}
	FbColor(radb_color_from_dBFS(peak_dBFS));
	int peak_height = (90 + peak_dBFS) * 1;
	if (peak_height < 1) peak_height = 1;
	FbMove(102, LCD_YSIZE - (8 + peak_height));
	FbRectangle(16, 1);

	int8_t avg_dBFS = audio_dBFS(mic_averages[i]);
	FbColor(radb_color_from_dBFS(avg_dBFS));
	int avg_height = (90 + avg_dBFS) * 1;
	if (avg_height < 1) avg_height = 1;
	FbMove(102, LCD_YSIZE - (8 + avg_height));
	FbFilledRectangle(16, avg_height);

	radv_b_for_back(button_down_latches());

        FbSwapBuffers();

	/* I don't know why, I don't want to know why, I shouldn't have to
	 * wonder why, but if this delay isn't here or at least 30 ms long the
	 * microphone stops working right within this app. -PMW */
	sleep_ms(30);
}

static void radv_color(void)
{
	/* Update color sample. */
	radv_color_sample();

	/* Clear the screen. */
	FbBackgroundColor(BLACK);
	FbClear();

	/* Write the banner */
	FbMove(8,8);
	FbColor(WHITE);
	FbWriteString("LIGHT\n\n");
	FbColor(PURPLE);
	FbWriteString("Bring me\nsomewhere...\n\n");

	/* Draw and evaluate todos. */
	radv_todos(TODO_COLOR_START, TODO_COLOR_END);

	int down_latches = button_down_latches();

	radv_b_for_back(down_latches);

        FbSwapBuffers();
}

void radv_conductivity(void)
{
	/* Clear the screen. */
	FbBackgroundColor(BLACK);
	FbClear();

	/* Write the banner */
	FbMove(8,8);
	FbColor(WHITE);
	FbWriteString("ELECTRONICS\n\n");
	FbColor(PURPLE);
	FbWriteString("Probe\nsomething...\n\n");

	/* Draw and evaluate todos. */
	radv_todos(TODO_CONDUCTIVITY_START, TODO_CONDUCTIVITY_END);

	int down_latches = button_down_latches();

	/* Press A to measure. */
	FbMove(8,LCD_YSIZE - 32);
	FbColor(GREY16);
	FbWriteLine("(A) Measure");
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		radv_conductivity_measure();
	}

	radv_b_for_back(down_latches);

        FbSwapBuffers();
}

void radv_magnometer(void)
{
	/* Measure mT */
	radv_magnometer_measure();

	/* Clear the screen. */
	FbBackgroundColor(BLACK);
	FbClear();

	/* Write the banner */
	FbMove(8,8);
	FbColor(WHITE);
	FbWriteString("MAGNETICS\n\n");
	FbColor(PURPLE);
	FbWriteString("Bring me a...\n\n");

	/* Draw and evaluate todos. */
	radv_todos(TODO_MAGNOMETER_START, TODO_MAGNOMETER_END);

	/* Draw magnometer vector. */
	int index = (m_magnet_index + ARRAY_SIZE(m_magnet_samples) - 1)
		    % ARRAY_SIZE(m_magnet_samples);
	int8_t mT = m_magnet_samples[index];
	FbColor(mT >= 0 ? RED : BLUE);
	FbLine(116,80,116,80 - (mT / 2));

	/* B for back! */
	int down_latches = button_down_latches();
	radv_b_for_back(down_latches);

        FbSwapBuffers();
}

void radv_temperature(void)
{
	/* Clear the screen. */
	FbBackgroundColor(BLACK);
	FbClear();

	/* Write the banner */
	FbMove(8,8);
	FbColor(WHITE);
	FbWriteString("TEMPERATURE\n\n");
	FbColor(PURPLE);
	FbWriteString("Bring me\nsomewhere...\n\n");

	/* Draw and evaluate todos. */
	radv_todos(TODO_TEMPERATURE_START, TODO_TEMPERATURE_END);

	/* Draw thermometer. */
	uint32_t mV = analog_get_chan_mV(ANALOG_CHAN_THERMISTOR);
	int8_t tC = analog_calc_thermistor_temp_C(mV);
	int8_t height = tC + 40; // Add 40 to keep it positive... if we are that cold!
	height = height > 1 ? height : 1; // lbound to pass positive to Fb
	FbColor(tC < 8 ? PURPLE : tC < 20 ? BLUE : tC > 40 ? RED : tC > 30 ? YELLOW : GREEN);
	FbMove(112, LCD_YSIZE - (8 + height));
	FbFilledRectangle(8, height);

	/* B for back! */
	int down_latches = button_down_latches();
	radv_b_for_back(down_latches);

        FbSwapBuffers();
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
	case ROVER_ADVENTURE_COLOR:
		radv_color();
		break;
	case ROVER_ADVENTURE_CONDUCTIVITY:
		radv_conductivity();
		break;
	case ROVER_ADVENTURE_MAGNOMETER:
		radv_magnometer();
		break;
	case ROVER_ADVENTURE_TEMPERATURE:
		radv_temperature();
		break;
	default:
		/* Always return to know good state through INIT */
		m_radv_state = ROVER_ADVENTURE_INIT;
		break;
	}
}


