#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "button.h"
#include "ir.h"
#include "colors.h"
#include "framebuffer.h"
#include "menu.h"
#include "audio.h"
#include "led_pwm.h"
#include "delay.h"
#include <accelerometer.h>
#include "music.h"

#include <utils.h>

#define LED_LVL 50

unsigned char QC_IR;

enum {
    INIT,
    RUN
};

void ir_callback(const IR_DATA * ir_data) {
    QC_IR = ir_data->data[0];
}

static unsigned char screen_brightness = 255;

struct qc_button {
    int button;
    const char *name;
    uint16_t freq;
    bool (*check)(const struct qc_button *b);
};

static bool check_button(const struct qc_button *b)
{
    if (0 == button_poll(b->button)) {
        /* Not pressed; nothing to do */
        return false;
    }

    char msg[16] = {0};
    snprintf(msg, sizeof(msg), "%s\n", b->name);
    FbWriteString(msg);

#define SUPPRESS_BEEP ((uint16_t) -1)
    if (b->freq != SUPPRESS_BEEP)
	    audio_out_beep(b->freq, 100);

    return true;
}

#if BADGE_HAS_ROTARY_SWITCHES
static bool check_encoder(const struct qc_button *b)
{
    int enc;
    if ((BADGE_BUTTON_ENCODER_A == b->button)
        || (BADGE_BUTTON_ENCODER_B == b->button)) {
        enc = 0;
    } else if ((BADGE_BUTTON_ENCODER_2_A == b->button)
               || (BADGE_BUTTON_ENCODER_2_B == b->button)) {
        enc = 1;
    } else {
        /* Not a valid button */
        return false;
    }

    int rotation = button_get_rotation(enc);
    if (rotation == 0) {
        /* No rotation; nothing to do */
        return false;
    }

    char msg[16] = {0};
    snprintf(msg, sizeof(msg), "%s %d\n", b->name, rotation);
    FbWriteString(msg);

    audio_out_beep(b->freq * (rotation > 0 ? 1 : 2), 100);

    return true;
}
#endif

static bool check_buttons(const struct qc_button *a, size_t n)
{
    bool any = false;

    for (const struct qc_button *b = a; b < (a + n); b++) {
        bool pressed = b->check(b);
        any |= pressed;
    }

    return any;
}

static const struct qc_button QC_BTN[] = {
    {BADGE_BUTTON_A, "A", 999, check_button},
    {BADGE_BUTTON_B, "B", SUPPRESS_BEEP, check_button},

    {BADGE_BUTTON_UP, "UP", 1046, check_button},
    {BADGE_BUTTON_DOWN, "DOWN", 740, check_button},
    {BADGE_BUTTON_LEFT, "LEFT", 784, check_button},
    {BADGE_BUTTON_RIGHT, "RIGHT", 932, check_button},
#if BADGE_HAS_ROTARY_BUTTONS
    {BADGE_BUTTON_ENCODER_SW, "ENC", 698, check_button},
    {BADGE_BUTTON_ENCODER_A, "ENC", 440, check_encoder},

    {BADGE_BUTTON_ENCODER_2_SW, "ENC 2", 777, check_button},
    {BADGE_BUTTON_ENCODER_2_A, "ENC 2", 666, check_encoder},
#endif
};

static struct note tune0[] = {
	{ NOTE_C4, 100 },
};

static struct note tune1[] = {
	{ NOTE_C4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_D4, 100 },
};

static struct note tune2[] = {
	{ NOTE_C4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_D4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_E4, 100 },
};

static struct note tune3[] = {
	{ NOTE_C4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_D4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_E4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_F4, 100 },
};

static struct note tune4[] = {
	{ NOTE_C4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_D4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_E4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_F4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_G4, 100 },
};

static struct note tune5[] = {
	{ NOTE_C4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_D4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_E4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_F4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_G4, 100 },
	{ NOTE_REST, 100 },
	{ NOTE_A4, 100 },
};

static struct tune accel_tune[] = {
	{ ARRAY_SIZE(tune0), tune0, },
	{ ARRAY_SIZE(tune1), tune1, },
	{ ARRAY_SIZE(tune2), tune2, },
	{ ARRAY_SIZE(tune3), tune3, },
	{ ARRAY_SIZE(tune4), tune4, },
	{ ARRAY_SIZE(tune5), tune5, },
};

static int trigger_accel_sound = 0;

static bool qc_accel(void)
{
    union acceleration a = accelerometer_last_sample();
    static char *axis_name[] = { "-X", "+X", "-Y", "+Y", "-Z", "+Z" };
    int max_axis = -1;
    int max_accel;

    char msg[64] = {0};
    snprintf(msg, sizeof(msg), "whoami:0x%02x\nx:%d\ny:%d\nz:%d\n",
             accelerometer_whoami(), a.x, a.y, a.z);
    FbWriteString(msg);

    max_accel = a.x;
    max_axis = a.x < 0 ? 0 : 1;
    if (abs(a.y) > abs(max_accel)) {
        max_accel = a.y;
	max_axis = a.y < 0 ? 2 : 3;
    }
    if (abs(a.z) > abs(max_accel)) {
        max_accel = a.z;
	max_axis = a.z < 0 ? 4 : 5;
	if (a.z < 0) {
		screen_brightness = 0;
		led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, screen_brightness);
	}
    }
    snprintf(msg, sizeof(msg), "Max Accel: %s\n", axis_name[max_axis]);
    FbWriteString(msg);

    if (trigger_accel_sound) {
	trigger_accel_sound = 0;
	play_tune(&accel_tune[max_axis], NULL);
    }

    return true;
}

void QC_cb(__attribute__((unused)) struct menu_t *menu)
{
    //static unsigned char call_count = 0;
    static int QC_state=0;
    static int button_hold_count = 0;
    unsigned char redraw = 0;
    IR_DATA ir_packet = {0};
    ir_packet.app_address = IR_APP0;
    ir_packet.recipient_address = IR_BADGE_ID_BROADCAST;
    ir_packet.data_length = 1;
    uint8_t data = 1;
    ir_packet.data = &data;

    switch(QC_state)
    {
        case INIT:
            ir_add_callback(ir_callback, IR_APP0);
            FbTransparentIndex(0);
            FbColor(GREEN);
            FbClear();

            FbMove(45, 45);
            FbWriteLine("QC!");
            FbMove(15, 55);
            FbWriteLine("Do things");
            FbSwapBuffers();
            QC_IR = 0;
            button_hold_count = 0;
            QC_state = RUN;
            break;

        case RUN:
            FbMove(16, 16);

#if BADGE_HAS_ROTARY_SWITCHES
            if (button_poll(BADGE_BUTTON_ENCODER_SW)) {
                button_hold_count ++;
            } else {
                button_hold_count = 0;
            }
#warning "Badge QC app probably needs maintenance around use of encoder switches"
#endif

            if(button_hold_count > 20){
                ir_remove_callback(ir_callback, IR_APP0);
                QC_state = INIT;
                FbWriteLine("EXITING");
                FbSwapBuffers();
                sleep_ms(1000);
                returnToMenus();
                return;
            }

            if (qc_accel())
            {
                redraw = 1;
            }

            if (check_buttons(QC_BTN, ARRAY_SIZE(QC_BTN)))
            {
                redraw = 1;
            }

            if (button_poll(BADGE_BUTTON_B)
                && button_poll(BADGE_BUTTON_DOWN)
#if BADGE_HAS_ROTARY_SWITCHES
                && button_poll(BADGE_BUTTON_ENCODER_SW)
                && button_poll(BADGE_BUTTON_ENCODER_2_SW)
#endif
		) {
                /* Force hard fault */
                data = *((uint8_t *) 0x8F000000);
            }

	    int down_latches = button_down_latches();

            if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		trigger_accel_sound = 1;
	    }

            // Send QC ping
            if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
                data = 1;
                ir_send_complete_message(&ir_packet);
            }

            // Received a QC ping
            if(QC_IR == 1){
                audio_out_beep(698 * 2, 400);
                FbColor(GREEN);
                FbWriteString("Pinged!\n");
                QC_IR = 0;
                data = 2;
                ir_send_complete_message(&ir_packet);
                redraw = 1;
            }
            // Received a QC ping response
            else if(QC_IR == 2){
                audio_out_beep(698 * 4, 200);
                FbColor(YELLOW);
                FbWriteString("Ping response!\n");
                FbColor(GREEN);
                QC_IR = 0;
                redraw = 1;
            }

	    FbMove(10, 130);
	    FbWriteString("PRESS AND HOLD\nRIGHT ROTARY\nSWITCH TO EXIT");

            if (redraw)
                FbSwapBuffers();

	    if (screen_brightness < 255) {
		screen_brightness++;
		led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, screen_brightness);
	    }
    }
}
