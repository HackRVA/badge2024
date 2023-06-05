#include <assert.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "accelerometer.h"
#include "xorshift.h"
#include "led_pwm.h"
#include "random.h"

static const char *magic_msg[] = {
	"IT IS\nCERTAIN\n",
	"IT IS\nDECIDELY\nSO\n",
	"WITHOUT\nA DOUBT\n",
	"YES\nDEFINITELY\n",
	"YOU MAY\nRELY ON\nIT\n",
	"AS I SEE\nIT, YES\n",
	"MOST\nLIKELY\n",
	"OUTLOOK\nGOOD\n",
	"YES\n",
	"SIGNS POINT\nTO YES\n",
	"REPLY HAZY\nTRY AGAIN\n",
	"ASK AGAIN\nLATER\n",
	"BETTER\nNOT TELL\nYOU NOW\n",
	"CANNOT\nPREDICT\nNOW\n",
	"CONCENTRATE\nAND ASK\nAGAIN\n",
	"DON'T COUNT\nON IT\n",
	"MY REPLY\nIS NO\n",
	"MY SOURCES\nSAY NO\n",
	"OUTLOOK\nNOT SO\nGOOD\n",
	"VERY\nDOUBTFUL\n",
};

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#define NMSGS (ARRAYSIZE(magic_msg))
static int current_message = -1;
static int screen_brightness = 255;

/* Program states.  Initial state is MAGIC8BALL_INIT */
enum magic_8_ball_state_t {
	MAGIC8BALL_INIT,
	MAGIC8BALL_RUN,
	MAGIC8BALL_EXIT,
};

static enum magic_8_ball_state_t magic_8_ball_state = MAGIC8BALL_INIT;
static int screen_changed = 0;

#define NSAMPLES 5
static union acceleration acc_sample[NSAMPLES];

/* return a random int between 0 and n - 1 */
static int random_num(int n)
{
	int x;
	static unsigned int state = 0;

	assert(n != 0);
	if (state == 0)
		random_insecure_bytes((uint8_t *) &state, sizeof(state));
	x = xorshift(&state);
	if (x < 0)
		x = -x;
	return x % n;
}

static void check_accelerometer(void)
{
	static int n = 0;
	union acceleration a;

	union acceleration acceleration = accelerometer_last_sample();

	/* Average the last few samples */
	acc_sample[n] = acceleration;
	n = n + 1;
	if (n >= NSAMPLES)
		n = 0;
	a.x = 0;
	a.y = 0;
	a.z = 0;
	for (int i = 0; i < NSAMPLES; i++) {
		a.x += acc_sample[i].x;
		a.y += acc_sample[i].y;
		a.z += acc_sample[i].z;
	}
	a.x /= NSAMPLES;
	a.y /= NSAMPLES;
	a.z /= NSAMPLES;

	if (a.z < -800) { /* screen facing floor, more or less, erase everything */
		/* turn screen brightness to zero */
		led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, 0);
		screen_brightness = 0;
		/* choose a new random message */
		current_message = random_num(NMSGS);
		screen_changed = 1;
	}

	if (a.z > 800) { /* screen facing up, more or less, fade in the brightness */
		led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, screen_brightness);
		screen_brightness++;
		if (screen_brightness > 255)
			screen_brightness = 255;
	}
}

static void magic_8_ball_init(void)
{
	FbInit();
	FbClear();
	magic_8_ball_state = MAGIC8BALL_RUN;
	screen_changed = 1;
	current_message = -1;
	screen_brightness = 0;
	led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, 0);
}

static int manually_triggered = 0;

static void check_buttons(void)
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches)) {
		magic_8_ball_state = MAGIC8BALL_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		current_message++;
		if (current_message >= (int) NMSGS)
			current_message = 0;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	}
	if ((button_poll(BADGE_BUTTON_A) && button_poll(BADGE_BUTTON_B))) {
		manually_triggered = 1;
	}

	if (manually_triggered) {
		if (screen_brightness < 255) {
			led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, screen_brightness);
			screen_brightness++;
			if (screen_brightness > 255) {
				screen_brightness = 255;
				manually_triggered = 0;
			}
		} else {
			if (manually_triggered) {
				screen_brightness = 0;
				led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, screen_brightness);
				/* choose a new random message */
				current_message = random_num(NMSGS);
				screen_changed = 1;
				manually_triggered = 0;
			}
		}
	}
}

static void draw_blue_triangle(void)
{
	int width = 110;
	FbColor(BLUE);
	for (int i = 0; i < 110; i++) {
		FbLine(64 - width / 2, i + 30, 64 + width / 2, i + 30);
		width--;
	}
}

static void draw_message(int n)
{
	char *m;
	char line[20];
	if (n == -1)
		m = (char *) "MAGIC\n8\nBALL\n";
	else
		m = (char *) magic_msg[n];

	int row = 40;
	int i = 0;
	int j = 0;
	FbColor(CYAN);
	FbBackgroundColor(BLUE);
	do {
		if (m[j] == '\0')
			break;
		line[i] = m[j];
		if (m[j] == '\n') {
			line[i] = '\0';
			int x = 64 - (i * 4);
			FbMove(x, row);
			FbWriteString(line);
			row += 9;
			i = 0;
			j++;
			continue;
		}
		i++;
		j++;
	} while (1);
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();
	draw_blue_triangle();
	draw_message(current_message);
	FbSwapBuffers();
	screen_changed = 0;
}

static void magic_8_ball_run(void)
{
	check_accelerometer();
	check_buttons();
	draw_screen();
}

static void magic_8_ball_exit(void)
{
	magic_8_ball_state = MAGIC8BALL_INIT; /* So that when we start again, we do not immediately exit */
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();
	FbSwapBuffers();
	/* Make sure the screen brightness is turned back up */
	led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, 255);
	returnToMenus();
}

void magic_8_ball_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (magic_8_ball_state) {
	case MAGIC8BALL_INIT:
		magic_8_ball_init();
		break;
	case MAGIC8BALL_RUN:
		magic_8_ball_run();
		break;
	case MAGIC8BALL_EXIT:
		magic_8_ball_exit();
		break;
	default:
		break;
	}
}

