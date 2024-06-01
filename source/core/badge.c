#include "colors.h"
#include "assetList.h"
#include "menu.h"
#include "button.h"
#include "screensavers.h"
#include "framebuffer.h"
#include "badge.h"
#include "random.h"
#include "led_pwm.h"
#include "display.h"
#include "ir.h"
#include "rtc.h"
#include "key_value_storage.h"
#include "settings.h"
#include "uid.h"
#include "xorshift.h"
#include "mic_pdm.h"

/*
  inital system data, will be save/restored from flash
*/

SYSTEM_DATA G_sysData = {
	.name={"               "}, 
	.badgeId=0, 
	.sekrits={ 0 }, 
	.achievements={ 0 },
	.ledBrightness=255,
	.backlight=192,
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

    flash_kv_get_binary("sysdata", badge_system_data(), sizeof(SYSTEM_DATA));
    
    G_sysData.badgeId = uid_get();

    led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, G_sysData.backlight);
    if (G_sysData.backlight < 5) {
        G_sysData.backlight = 5;
    }
    led_pwm_set_scale(G_sysData.ledBrightness);
    display_set_rotation(G_sysData.display_rotated);
    if (G_sysData.display_inverted) {
        display_set_display_mode_inverted();
    }

    setup_settings_menus();
}


// dormant returns 1 if touch/buttons and IR messages are dormant for 60 seconds, otherwise returns 0
unsigned char dormant(void) {
    if (mic_running()) {
        // Going dormant with the mic running seems to cause problems -PMW
        return 0;
    }
    uint32_t timestamp = (uint32_t)rtc_get_ms_since_boot();
    if (timestamp >= (button_last_input_timestamp() + 1000 * 60)){
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
unsigned char current_screen_saver = 0;

#define HIGH_PROB_THRESH 100
#define MEDIUM_PROB_THRESH 30
#define LOW_PROB_THRESH 15

void do_screen_save_popup(void)
{

    static unsigned char prob_val = 50;
    static unsigned int xorshift_state = 0xa5a5a5a5;

    if (badge_system_data()->screensaver_disabled)
	return;

    if(popup_time == POPUP_LENGTH) {
	prob_val = xorshift(&xorshift_state) % 100;
    }
    
    if(popup_time){
        
        if(prob_val < MEDIUM_PROB_THRESH){
 #if 0
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
#endif
        }
        else if(prob_val < HIGH_PROB_THRESH){
            switch(current_screen_saver%10){
                case 0:
                case 1:
                case 3:
		case 8:
                    disp_asset_saver();
                    break;
                case 2:
                    dotty();
                    break;
                case 4:
                    just_the_badge_tips(); /* need to rework this */
                    break;
                case 5:
                    qix();
                    break;
                case 6:
                    matrix();
                    break;
                case 7:
                    hyperspace_screen_saver();
                    break;
                case 9:
                    nametag_screensaver();
                    break;
            }
        }
        popup_time--;
    }
    else{ // stop the popup!
        FbClear();
        popup_time = POPUP_LENGTH;
        screen_save_popup_cnt = SCREEN_SAVE_POPUP_DELAY;
        screensaver_set_animation_count(0);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_disable(BADGE_LED_RGB_GREEN);
        led_pwm_disable(BADGE_LED_RGB_BLUE);
        current_screen_saver++; // TODO: add randomness to this
        //current_screen_saver = timestamp%5;
    }
    
}

/* is_dormant is 1 if the screen saver has been activated
 * due to lack of button presses or IR messages, 0 otherwise */
unsigned char is_dormant = 0;
unsigned char screen_save_lockout = 0;

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
	FbBackgroundColor(BLACK);
        FbSwapBuffers();
    }
    
    if(is_dormant){

        if (!dormant() || ir_messages_seen(false)) {
            //|| (IRpacketInCurr != IRpacketInNext)){
            is_dormant = 0;
            ir_messages_seen(true);
	    display_reset(); // Just to be safe, put back to known good state.
            brightScreen = 1;
            led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, G_sysData.backlight);
            popup_time = POPUP_LENGTH;
            screen_save_popup_cnt = SCREEN_SAVE_POPUP_DELAY;
	    if (runningApp == NULL)
		    menu_redraw_main_menu = 1; //hack
            //reset timer
            button_reset_last_input_timestamp();
            
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
