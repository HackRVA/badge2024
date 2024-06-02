#include <stdint.h>
#include <stdbool.h>

#include "audio.h"
#include "button.h"
#include "colors.h"
#include "display.h"
#include "framebuffer.h"
#include "led_pwm.h"
#include "menu.h"
#include "music.h"
#include "rvasec_splash.h"
#include "hackrvanewlogo.h"
#include "utils.h"

static const char splash_words1[] = "Loading";
#define NUM_WORD_THINGS ARRAY_SIZE(splash_word_things)
static const char *splash_word_things[] = {
    "Cognition Module",
    "useless bits",
    "backdoor.sh",
    "exploit inside",
    "sub-zero",
    "lifting tables",
    "personal data",
    "important bits",
    "bitcoin miner",
    "advanced AI(tm)",
    "broken feature",
    "NTFS",
    "ZFS",
    "BTRFS",
    "All the FS",
    "Wall hacks",
    "huawei 5G",
    "Key logger",
    "badgedows defender",
    "sshd",
    "cryptolocker",
};

static const char splash_words_btn1[] = "Press any button";
static const char splash_words_btn2[] = "to continue!";

static void brand_preproduction_firmware(int blink)
{
	(void) blink;
#define PREPRODUCTION_FIRMWARE 0
#if PREPRODUCTION_FIRMWARE
	if (!blink)
		return;
	FbColor(RED);
	FbMove(13, 25);
	FbWriteString("ALPHA FIRMWARE");
	FbPushBuffer();
#endif
}

#define SPLASH_SHIFT_DOWN 85
void rvasec_splash_cb(__attribute__((unused)) struct menu_t *m)
{
    extern const struct asset2 RVAsec_13;
    static unsigned short wait = 0;
    static unsigned char loading_txt_idx = 0,
    load_bar = 0;

    if (wait == 0) {
        load_bar = 10;
        display_rect(0, 0, LCD_XSIZE, LCD_YSIZE);
        display_color(0);
        FbSwapBuffers();
        led_pwm_enable(BADGE_LED_RGB_GREEN, 50 * 255/100);
        //if(buzzer)
        audio_out_beep(NOTE_C3, 50);
    } else if(wait < 40){
        FbImage4bit2(&hackrva_badge_logo, 0);
        FbSwapBuffers();
        //PowerSaveIdle();
    } else if(wait < 80){
        FbMove(0, 0);
        FbImage2(&RVAsec_13, 0);
        FbMove(10,SPLASH_SHIFT_DOWN);

        FbColor(WHITE);
        FbRectangle(100, 20);

        FbMove(35, SPLASH_SHIFT_DOWN - 13);
        FbColor(YELLOW);
        FbWriteLine(splash_words1);

        FbMove(11, SPLASH_SHIFT_DOWN+1);
        FbColor(GREEN);
        FbFilledRectangle((load_bar++ << 1) + 1,19);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 10 * 255 / 100);

        FbColor(WHITE);
        FbMove(4, 113);
        FbWriteLine(splash_word_things[loading_txt_idx%NUM_WORD_THINGS]);
        if(!(wait%2))
            loading_txt_idx++;

        FbSwapBuffers();

    } else if(wait <160){
        FbMove(0, 0);
        FbImage2(&RVAsec_13, 0);
#if 0
	/* Draw the griddy horizon thing */
        FbMove(10,SPLASH_SHIFT_DOWN);
        FbColor(GREEN);
        FbLine(0,60,132,60);
        FbLine(0,62,132,62);
        FbLine(0,65,132,65);
        FbLine(0,69,132,69);
        FbLine(0,77,132,77);

        FbLine(105,60,145,77);
        FbLine(95, 60,125,77);
        FbLine(85, 60,105,77);
        FbLine(75, 60,85,77);
        FbLine(65, 60,65,77);
        FbLine(55, 60,45,77);
        FbLine(45, 60,25,77);
        FbLine(35, 60,5,77);
        FbLine(25, 60,0,65);
#endif

        FbColor(RED);
	FbBackgroundColor(0x21c5);
        FbMove(1, 90);
        FbWriteLine(splash_words_btn1);

        FbMove(15, 110);
        FbWriteLine(splash_words_btn2);

        brand_preproduction_firmware(!((wait / 5) & 0x01) || wait > 158);
	FbBackgroundColor(BLACK);
        FbSwapBuffers();
    } else {
        int down_latches = button_down_latches();
        if (0 != down_latches) {
            FbBackgroundColor(BLACK);
            returnToMenus();
        }
    }

    wait++;
    /* Don't let wait overflow, or else the splash animation will start over. */
    if (wait == 0)
        wait -= 1000;

}


