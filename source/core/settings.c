#include "string.h"
#include "badge.h"
#include "menu.h"
#include "button.h"
#include "ir.h"
#include "framebuffer.h"
#include "delay.h"
#include "led_pwm.h"
#include "display_s6b33.h"

#define PING_REQUEST      0x1000
#define PING_RESPONSE     0x2000

void ping_cb(){
    static unsigned char num_pinged = 0;

    if(!num_pinged){
        FbClear();
        FbSwapBuffers();
        //FbMove(50, 100);
        //FbWriteLine("No response");

        IR_DATA ir_packet;
        ir_packet.app_address = IR_PING;
        ir_packet.recipient_address = 0;
        ir_packet.data_length = 2;
        uint8_t data[2];
        data[0] = PING_REQUEST >> 8;
        data[1] = PING_REQUEST & 0xFF;
        ir_packet.data = data;

        ir_send_complete_message(&ir_packet);
        num_pinged++;
    }

    // TODO: Implement a callback function to receive pings and detect responses.
#if 0
    if(ping_responded) {
        unsigned char bid[4] = {'0' + ping_responded / 100 % 10,
                                '0' + ping_responded / 10 % 10,
                                '0' + ping_responded % 10,
                                0};

        FbMove(10, 10 + ((num_pinged - 1) * 10));
        FbWriteLine(bid);
        FbPushBuffer();
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
        ping_responded = 0;
        sleep_ms(10) ; // wait a bit
    }
#else
    sleep_ms(10);
#endif

    returnToMenus();
}

const struct menu_t ping_m[] = {
    {"Ping",   VERT_ITEM, FUNCTION, {(struct menu_t *)ping_cb} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};


/*
    set badge Id
*/
void myBadgeid_cb(struct menu_t *h) {
   struct menu_t *selectedMenu;

   //dstMenu = getSelectedMenuStack(1);
   selectedMenu = getSelectedMenu();

   uint16_t badge_id = badge_system_data()->badgeId;

   selectedMenu->name[0] = hextab[((badge_id >> 12) & 0xF)];
   selectedMenu->name[1] = hextab[((badge_id >>  8) & 0xF)];
   selectedMenu->name[2] = hextab[((badge_id >>  4) & 0xF)];
   selectedMenu->name[3] = hextab[((badge_id      ) & 0xF)];
   selectedMenu->name[4] = 0;
   //strcpy(dstMenu->name, selectedMenu->name);
   returnToMenus();
}

const struct menu_t myBadgeid_m[] = {
    {"check",   VERT_ITEM, FUNCTION, {(struct menu_t *)myBadgeid_cb} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

/*
    peer we want to talk to
*/

// Sam: This seems unused... Perhaps safe to remove?
void peerBadgeid_cb(struct menu_t *h) {
   struct menu_t *dstMenu, *selectedMenu;

   dstMenu = getSelectedMenuStack(1);
   selectedMenu = getSelectedMenu();
   strcpy(dstMenu->name, selectedMenu->name);
}


const struct menu_t peerBadgeNum_m[] = {
    {"ALL", 0|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  1", 1|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  2", 2|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  3", 3|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  4", 4|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  5", 5|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  6", 6|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  7", 7|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"  8", 8|VERT_ITEM, FUNCTION, {(struct menu_t *)peerBadgeid_cb} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

struct menu_t peerBadgeid_m[] = {
    {"0",   VERT_ITEM, MENU, {peerBadgeNum_m} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};


/*
    backlight
*/
void backlight_cb(struct menu_t *h) {
   struct menu_t *dstMenu, *selectedMenu;

   dstMenu = getSelectedMenuStack(1);
   selectedMenu = getSelectedMenu();

   /* update calling menu's name field */
   strcpy(dstMenu->name, selectedMenu->name);

   badge_system_data()->backlight = selectedMenu->attrib & 0x1FF;
   led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, badge_system_data()->backlight);
   //flashWriteKeyValue((unsigned int)&G_sysData, (char *)&G_sysData, sizeof(struct sysData_t));

   returnToMenus();
}

const struct menu_t backlightList_m[] = {
//    {"       ", 0|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} }, oh support... why is my screen black?
    {"      -",  16|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} },
    {"     --",  32|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} },
    {"    ---",  64|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} },
    {"   ----", 128|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} },
    {"  -----", 192|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} },
    {" ------", 224|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} },
    {"-------", 255|VERT_ITEM, FUNCTION, {(struct menu_t *)backlight_cb} },

    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

struct menu_t backlight_m[] = {
    {"-------",   VERT_ITEM,     MENU, {backlightList_m} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

int getRotate(void)
{
	return S6B33_get_rotation();
}

void setRotate(int yes)
{
    S6B33_set_rotation(yes);
}

/*
   rotate screen
*/
void rotate_cb(struct menu_t *h) {
    unsigned char rotated=0;
    struct menu_t *selectedMenu;
    selectedMenu = getSelectedMenu();

    rotated = selectedMenu->attrib & 0x1FF;

    setRotate(rotated);

    returnToMenus();
};

extern void S6B33_set_display_mode_inverted(void);
extern void S6B33_set_display_mode_noninverted(void);

static void invert_cb(struct menu_t *h) {
	static int inverted = 0;
	inverted = !inverted;
	if (inverted)
		S6B33_set_display_mode_inverted();
	else
		S6B33_set_display_mode_noninverted();
	returnToMenus();
}

const struct menu_t rotate_m[] = {
    {"Default",   0|VERT_ITEM, FUNCTION, {(struct menu_t *)rotate_cb} },
    {"Rotated",   1|VERT_ITEM, FUNCTION, {(struct menu_t *)rotate_cb} },
    {"Inverted",   1|VERT_ITEM, FUNCTION, {(struct menu_t *)invert_cb} },
    {"Back",      VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};



/*
    LED brightness
*/
void LEDlight_cb(struct menu_t *h) {
    struct menu_t *dstMenu, *selectedMenu;

    dstMenu = getSelectedMenuStack(1);
    selectedMenu = getSelectedMenu();

    strcpy(dstMenu->name, selectedMenu->name);

    badge_system_data()->ledBrightness = selectedMenu->attrib & 0x7;
    //flashWriteKeyValue((unsigned int)&G_sysData, (char *)&G_sysData, sizeof(struct sysData_t));

    /* because of calcs done on pwm, 
       have to reload the values for
       it to take effect. because of division, 
       info is lost in this process
    */

    // Sam: this used to point to set some global variables; perhaps this was config? TODO check this out.
    led_pwm_enable(BADGE_LED_RGB_RED, 127);
    led_pwm_enable(BADGE_LED_RGB_GREEN, 127);
    led_pwm_enable(BADGE_LED_RGB_BLUE, 127);

    returnToMenus();
}


const struct menu_t LEDlightList_m[] = {
//    {"       ", 7|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {"      -", 6|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {"     --", 5|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {"    ---", 4|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {"   ----", 3|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {"  -----", 2|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {" ------", 1|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {"-------", 0|VERT_ITEM, FUNCTION, {(struct menu_t *)LEDlight_cb} },
    {"Back", VERT_ITEM|LAST_ITEM| DEFAULT_ITEM, BACK, {NULL} },
};

struct menu_t LEDlight_m[] = {
    {"--------",   VERT_ITEM,     MENU, {LEDlightList_m} },
    {"Back", VERT_ITEM|LAST_ITEM| DEFAULT_ITEM, BACK, {NULL} },
};

void buzzer_config_cb()
{
    struct menu_t *dstMenu, *selectedMenu;

    dstMenu = getSelectedMenuStack(1); /* parent menu */
    selectedMenu = getSelectedMenu();

    strcpy(dstMenu->name, selectedMenu->name);

    badge_system_data()->mute = selectedMenu->attrib & 0x1; /* low order bits of attrib can store values */
    //flashWriteKeyValue((unsigned int)&G_sysData, (char *)&G_sysData, sizeof(struct sysData_t));

    returnToMenus();
}

const struct menu_t buzzer_config_m[] = {
    {"Buzzer: On",   0|VERT_ITEM,     FUNCTION, {(struct menu_t *)buzzer_config_cb} },
    {"Buzzer: Off",  1|VERT_ITEM,     FUNCTION, {(struct menu_t *)buzzer_config_cb} },
    {"Back", VERT_ITEM|LAST_ITEM| DEFAULT_ITEM, BACK, {NULL} },
};

/*
  not const menu_t ...  because the config status is stored in buzzer_m[0].name[]
*/
struct menu_t buzzer_m[] = {
    {"Buzzer: On",   VERT_ITEM,     MENU, {buzzer_config_m} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

extern unsigned char screen_save_lockout;
extern unsigned char screensaver_inverted;

void screen_save_lock_cb(struct menu_t *h) {
    struct menu_t *selectedMenu;
    selectedMenu = getSelectedMenu();
    screen_save_lockout = selectedMenu->attrib & 0x1FF;
    returnToMenus(); 
}

void screen_save_invert_cb() {
    screensaver_inverted = !screensaver_inverted;
    returnToMenus();
}

const struct menu_t screen_lock_m[] = {
    {"ON",       0|VERT_ITEM, FUNCTION, {(struct menu_t *)screen_save_lock_cb} },
    {"OFF",      1|VERT_ITEM, FUNCTION, {(struct menu_t *)screen_save_lock_cb} },
    {"INVERT", VERT_ITEM, FUNCTION, {(struct menu_t *)screen_save_invert_cb} },
    {"Back",   VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};


