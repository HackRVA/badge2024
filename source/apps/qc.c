#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "button.h"
#include "ir.h"
#include "colors.h"
#include "framebuffer.h"
#include "menu.h"
#include "audio.h"
#include "led_pwm.h"
#include "delay.h"
#include "music.h"
#include "analog.h"
#include "color_sensor.h"
#include "mic_pdm.h"
#include "rtc.h"

#include <utils.h>

#define LED_LVL 50

static unsigned char QC_IR;
static unsigned QC_IR_frames;

enum {
    INIT,
    RUN
};

void ir_callback(const IR_DATA * ir_data) {
    QC_IR_frames = 5;
    QC_IR = ir_data->data[0];
}

#define SUPPRESS_BEEP ((uint16_t) -1)

struct qc_button {
    int button;
    const char *name;
    uint16_t freq;
    bool (*check)(const struct qc_button *b);
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

static bool check_button(const struct qc_button *b)
{
    if (0 == button_poll(b->button)) {
        /* Not pressed; nothing to do */
        return false;
    }

    char msg[16] = {0};
    int len = snprintf(msg, sizeof(msg), "%s\n", b->name);
    FbWriteString(msg);
    printf("%.*s\t", len - 1, msg);

    if (b->freq != SUPPRESS_BEEP)
	    audio_out_beep(b->freq, 100);

    led_pwm_enable(BADGE_LED_RGB_RED, b->r);
    led_pwm_enable(BADGE_LED_RGB_GREEN, b->g);
    led_pwm_enable(BADGE_LED_RGB_BLUE, b->b);

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
    {BADGE_BUTTON_A, "A", 999, check_button, 0, 255, 0},
    {BADGE_BUTTON_B, "B", 698, check_button, 0, 0, 255},

    {BADGE_BUTTON_UP, "UP", 1046, check_button, 255, 0, 0},
    {BADGE_BUTTON_DOWN, "DOWN", 740, check_button, 255, 127, 0},
    {BADGE_BUTTON_LEFT, "LEFT", 784, check_button, 127, 127, 0},
    {BADGE_BUTTON_RIGHT, "RIGHT", 932, check_button, 127, 255, 0},
#if BADGE_HAS_ROTARY_BUTTONS
    {BADGE_BUTTON_ENCODER_SW, "ENC", 698, check_button},
    {BADGE_BUTTON_ENCODER_A, "ENC", 440, check_encoder},

    {BADGE_BUTTON_ENCODER_2_SW, "ENC 2", 777, check_button},
    {BADGE_BUTTON_ENCODER_2_A, "ENC 2", 666, check_encoder},
#endif
};

bool qc_analog(void)
{
    int len;
    char msg[16];

    float ohms = analog_calc_resistance_ohms(analog_get_chan_mV(ANALOG_CHAN_CONDUCTIVITY));
    len = snprintf(msg, sizeof(msg), "R:%1.1e\n", ohms);
    FbWriteString(msg);
    printf("%.*s\t", len - 1, msg);

    int8_t therm_C = analog_calc_thermistor_temp_C(analog_get_chan_mV(ANALOG_CHAN_THERMISTOR));
    len = snprintf(msg, sizeof(msg), "ThermC:%3d\n", therm_C);
    FbWriteString(msg);
    printf("%.*s\t", len - 1, msg);
    
    int32_t hall_effect_mT = analog_calc_hall_effect_mT(analog_get_chan_mV(ANALOG_CHAN_HALL_EFFECT));
    len = snprintf(msg, sizeof(msg), "HallmT:%3d\n", hall_effect_mT);
    FbWriteString(msg);
    printf("%.*s\t", len - 1, msg);

    uint32_t batt_mV = analog_get_batt_mV();
    len = snprintf(msg, sizeof(msg), "BattV:%1d.%02d\n", batt_mV / 1000, batt_mV % 1000 / 10);
    FbWriteString(msg);
    printf("%.*s\t", len - 1, msg);

    int8_t mcu_temp = analog_calc_mcu_temp_C(analog_get_chan_mV(ANALOG_CHAN_MCU_TEMP));
    len = snprintf(msg, sizeof(msg), "MCUTC:%d\n", mcu_temp);
    FbWriteString(msg);
    printf("%.*s\t", len - 1, msg);

    return true;
}

bool qc_color_sensor(void) {
    int len;
    char msg[15];
    struct color_sample sample;
    int rc = color_sensor_get_sample(&sample);
    if (rc < 0) {
        len = snprintf(msg, sizeof(msg), "cls err:%04x\n", 
                 color_sensor_get_error_code());
        FbWriteString(msg);
        printf("%.*s\t", len - 1, msg);
        return true;
    }
    
    const char color[COLOR_SAMPLE_INDEX_COUNT] = {'r', 'g', 'b', 'w', 'i'};
    const unsigned short text_color[COLOR_SAMPLE_INDEX_COUNT] = {
	    RED, GREEN, BLUE, WHITE, PACKRGB(16,0,8)
    };
    for (int i = 0; i < COLOR_SAMPLE_INDEX_COUNT; i++) {
        FbColor(text_color[i]);
        len = snprintf(msg, sizeof(msg), "%c:%d\n", color[i],
                       sample.error_flags & (1U << i) ? -1 : sample.rgbwi[i]);
        FbWriteString(msg);
        printf("%.*s\t", len - 1, msg);
    }
    FbColor(GREEN);
    
    return true;
}

bool qc_mic(void)
{
    static uint8_t long_average_idx = 0;
    static audio_sample_t long_average[16] = {0};
    int len;
    char msg[16];

    long_average[long_average_idx] = mic_get_qc_value();
    long_average_idx++;
    long_average_idx %= ARRAY_SIZE(long_average);

    int8_t dB = audio_dBFS(audio_rms(long_average, ARRAY_SIZE(long_average)));
    len = snprintf(msg, sizeof(msg), "Mic:%03d\n", dB);
    FbWriteString(msg);
    printf("%.*s\t", len - 1, msg);

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
            analog_set_sensor_power(ANALOG_SENSOR_POWER_ENABLED);
            //color_sensor_power_ctl(COLOR_SENSOR_POWER_CMD_UP);
            ir_add_callback(ir_callback, IR_APP0);
            mic_start();
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
	    /* Reset every frame. */
            FbTransparentIndex(0);
            FbColor(GREEN);
            FbClear();
            FbMove(8, 8);
	    printf("\r\nqc:\t");

            if (button_poll(BADGE_BUTTON_UP) && button_poll(BADGE_BUTTON_B)) {
                button_hold_count ++;
            } else {
                button_hold_count = 0;
            }

            if(button_hold_count > 20){
		/* Exit */
                analog_set_sensor_power(ANALOG_SENSOR_POWER_DISABLED);
                //color_sensor_power_ctl(COLOR_SENSOR_POWER_CMD_DOWN);
                ir_remove_callback(ir_callback, IR_APP0);
                mic_stop();
		led_pwm_disable(BADGE_LED_RGB_RED);
		led_pwm_disable(BADGE_LED_RGB_GREEN);
		led_pwm_disable(BADGE_LED_RGB_BLUE);
                QC_state = INIT;
                FbWriteLine("EXITING");
                FbSwapBuffers();
                sleep_ms(500);
                returnToMenus();
                return;
            }

            if (qc_analog())
            {
                redraw = 1;
            }

            if (qc_color_sensor()) {
                redraw = 1;
            }

            if (qc_mic()) {
                redraw = 1;
            }

            if (check_buttons(QC_BTN, ARRAY_SIZE(QC_BTN)))
            {
                redraw = 1;
            }

            if (button_poll(BADGE_BUTTON_B)
                && button_poll(BADGE_BUTTON_DOWN)
                && button_poll(BADGE_BUTTON_A)
		) {
                /* Force hard fault */
                data = *((uint8_t *) 0x8F000000);
            }

	    int down_latches = button_down_latches();

            if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
                // Do nothing
	    }

            // Send QC ping
            if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
                data = 1;
                ir_send_complete_message(&ir_packet);
            }

            // Received a QC ping
            if(QC_IR == 1){
                audio_out_beep(698 * 2, 400);
                FbColor(YELLOW);
                FbWriteString("Pinged!\n");
                printf("Pinged!\t");
                FbColor(GREEN);
                if (QC_IR_frames == 0) {
                    QC_IR = 0;
                    data = 2;
                    ir_send_complete_message(&ir_packet);
                } else {
                    QC_IR_frames--;
                }
                redraw = 1;
            }
            // Received a QC ping response
            else if(QC_IR == 2){
                audio_out_beep(698 * 4, 200);
                FbColor(YELLOW);
                FbWriteString("Ping response!\n");
                printf("Ping response!\t");
                FbColor(GREEN);
                if (QC_IR_frames == 0) {
                    QC_IR = 0;
                } else {
                    QC_IR_frames--;
                }
                redraw = 1;
            }

            // test the clock
	    char buf[25];
	    snprintf(buf, sizeof(buf), "%" PRIu64 "\n", rtc_get_ms_since_boot());
	    FbWriteString(buf);

	    FbMove(8, 128);
	    FbWriteString("PRESS AND HOLD\nUP and B\nTO EXIT");

            if (redraw)
                FbSwapBuffers();
    }
}
