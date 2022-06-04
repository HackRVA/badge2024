#include "colors.h"
#include "assetList.h"
#include "menu.h"
#include "button.h"
#include "assets.h"
#include "screensavers.h"
#include "framebuffer.h"
#include "badge.h"
#include "random.h"
#include "led_pwm.h"
#include "display_s6b33.h"
#include "ir.h"
#include "rtc.h"
#include "key_value_storage.h"
#include "settings.h"

/*
  inital system data, will be save/restored from flash
*/
const char hextab[16]={"0123456789ABCDEF"};

#ifndef INITIAL_BADGE_ID
#define INITIAL_BADGE_ID (0xADDE)
#endif

/* use only for final script has to be CONST to be found in hex */
const unsigned short finalBadgeId = INITIAL_BADGE_ID; 

SYSTEM_DATA G_sysData = {
	.name={"               "}, 
	.badgeId=0, 
	.sekrits={ 0 }, 
	.achievements={ 0 },
	.ledBrightness=255,
	.backlight=255,
	.mute=0
};

unsigned char  NextUSBOut=0;


SYSTEM_DATA* badge_system_data(void) {
    return &G_sysData;
}

void UserInit(void)
{
    flash_kv_init();
    FbInit();
    FbClear();

    /* if not in flash, use the default assigned by make */
    G_sysData.badgeId = finalBadgeId;

    flash_kv_get_binary("sysdata", badge_system_data(), sizeof(SYSTEM_DATA));

    led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, G_sysData.backlight);
    if (G_sysData.backlight < 5) {
        G_sysData.backlight = 5;
    }
    led_pwm_set_scale(G_sysData.ledBrightness);
    S6B33_set_rotation(G_sysData.display_rotated);
    if (G_sysData.display_inverted) {
        S6B33_set_display_mode_inverted();
    }

    setup_settings_menus();
}


// dormant returns 1 if touch/buttons are dormant for 2 minutes, otherwise returns 0
unsigned char dormant(void) {
    uint32_t timestamp = (uint32_t)rtc_get_ms_since_boot();
    if (timestamp >= (button_last_input_timestamp() + 1000 * 2 * 60)){
        if(!ir_messages_seen(false)){
            return 1;
        }
        else {
            // wake on IR receive and reset timer
            button_reset_last_input_timestamp();
            return 0;
        }
    } 
    else {
        return 0;
    }
}

// in frames
#define SCREEN_SAVE_POPUP_DELAY (10 * 1000 / 30)
unsigned int screen_save_popup_cnt = SCREEN_SAVE_POPUP_DELAY;

// in frames
#define POPUP_LENGTH (9 * 30)
unsigned short popup_time = POPUP_LENGTH;

unsigned char brightScreen = 1;
extern unsigned short anim_cnt;
unsigned char current_screen_saver = 0;
extern unsigned char redraw_main_menu;

#define HIGH_PROB_THRESH 100
#define MEDIUM_PROB_THRESH 30
#define LOW_PROB_THRESH 15

void do_screen_save_popup(){

    static unsigned char prob_val = 50;
    if(popup_time == POPUP_LENGTH) {
        random_insecure_bytes(&prob_val, 1);
        prob_val %= 100;
    }
    
    if(popup_time){
        
        if(prob_val < MEDIUM_PROB_THRESH){
            
            switch(current_screen_saver%4){
                case 0:
                    smiley();
                    break;
                case 1:
                    bluescreen();
                    break;
                case 2:
                    hack_the_dragon();
                    break;
                case 3:
                    for_president();
                    break;
            }
        }
        else if(prob_val < HIGH_PROB_THRESH){
            switch(current_screen_saver%5){
                case 0:
                    disp_asset_saver();
                    break;
                case 1:
                    stupid_rects();
                    break;
                case 2:
                    dotty();
                    break;
                case 3:
                    carzy_tunnel_animator();
                    break;
                case 4:
                    just_the_badge_tips();
                    break;
                    
            }
        }
        popup_time--;
    }
    else{ // stop the popup!
        FbClear();
        popup_time = POPUP_LENGTH;
        screen_save_popup_cnt = SCREEN_SAVE_POPUP_DELAY;
        anim_cnt = 0;
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_disable(BADGE_LED_RGB_GREEN);
        led_pwm_disable(BADGE_LED_RGB_BLUE);
        current_screen_saver++; // TODO: add randomness to this
        //current_screen_saver = timestamp%5;
    }
    
}
unsigned char is_dormant = 0;
unsigned char screen_save_lockout = 0;

unsigned char screensaver_inverted = 0;

uint64_t ProcessIO(void)
{
    // 30 fps
    static const uint64_t frame_interval_us_default = 1000000/30;

    /*
	this ProcessIO() is the badge main loop
	buttons are serviced only when the app finishes
	same with IR events and USB input/output
    */

    //IRhandler(); /* do any pending IR callbacks */
    menus();

    //initialize assuming the badge is active (backlight is powered);
    brightScreen = 1;
    
    if(dormant() && !is_dormant && !screen_save_lockout) {
        is_dormant = 1;
        // Turn off LED to allow sleep modes
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_disable(BADGE_LED_RGB_BLUE);
        led_pwm_disable(BADGE_LED_RGB_GREEN);
        FbClear();
        FbColor(BLACK);
        FbSwapBuffers();
        if(!screensaver_inverted) {
            if(S6B33_get_display_mode() == DISPLAY_MODE_NORMAL) {
                S6B33_set_display_mode_inverted();
            }
            else {
                S6B33_set_display_mode_noninverted();    
            }
        }
    }
    
    if(is_dormant){

        if (!dormant() || ir_messages_seen(false)) {
            //|| (IRpacketInCurr != IRpacketInNext)){
            is_dormant = 0;
            ir_messages_seen(true);
            brightScreen = 1;
            led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, G_sysData.backlight);
            popup_time = POPUP_LENGTH;
            screen_save_popup_cnt = SCREEN_SAVE_POPUP_DELAY;
            redraw_main_menu = 1;//hack
            //reset timer
            button_reset_last_input_timestamp();
            
            if(!screensaver_inverted) {
                if(S6B33_get_display_mode() == DISPLAY_MODE_NORMAL) {
                    S6B33_set_display_mode_inverted();
                }
                else {
                    S6B33_set_display_mode_noninverted();    
                }
            }
            
            return frame_interval_us_default;
        }
        else if (brightScreen){
            led_pwm_disable(BADGE_LED_DISPLAY_BACKLIGHT);
            brightScreen = 0;
        }
            
        if(screen_save_popup_cnt){
            screen_save_popup_cnt--;
            //PowerSaveIdle();
        }
        else if(!screen_save_popup_cnt){
            if(!brightScreen){
                brightScreen = 1;
                led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, G_sysData.backlight);
            }
            do_screen_save_popup();
        }
    }

    return frame_interval_us_default;
}
