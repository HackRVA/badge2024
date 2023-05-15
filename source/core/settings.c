#include "string.h"
#include "badge.h"
#include "menu.h"
#include "button.h"
#include "ir.h"
#include "framebuffer.h"
#include "delay.h"
#include "led_pwm.h"
#include "display.h"
#include "key_value_storage.h"
#include "test-screensavers.h"

#define PING_REQUEST      0x1000
#define PING_RESPONSE     0x2000

static void save_settings(void) {
    flash_kv_store_binary("sysdata", badge_system_data(), sizeof(SYSTEM_DATA));
}

void ping_cb(__attribute__((unused)) struct menu_t *menu)
{
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
    {"Ping",   VERT_ITEM, FUNCTION, { .func = ping_cb} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};


/*
    set badge Id
*/
static const char *hextab = "0123456789ABCDEF";
void myBadgeid_cb(__attribute__((unused)) struct menu_t *h) {
    struct menu_t *selectedMenu;

    //dstMenu = getSelectedMenuStack(1);
    selectedMenu = getSelectedMenu();

    uint64_t badge_id = badge_system_data()->badgeId;

    size_t i = 15;
    selectedMenu->name[15] = 0;
    for(; i > 0; i--)
    {
        selectedMenu->name[15-i] = hextab[(badge_id >> 4 * (i - 1)) & 0xF];
    }

    //strcpy(dstMenu->name, selectedMenu->name);
    returnToMenus();
}

struct menu_t myBadgeid_m[] = {
    {"check",   VERT_ITEM, FUNCTION, { .func = myBadgeid_cb} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};


/*
    backlight
*/
void backlight_cb(__attribute__((unused)) struct menu_t *h) {
   struct menu_t *dstMenu, *selectedMenu;

   dstMenu = getSelectedMenuStack(1);
   selectedMenu = getSelectedMenu();

   /* update calling menu's name field */
   strcpy(dstMenu->name, selectedMenu->name);

   badge_system_data()->backlight = selectedMenu->attrib & 0x1FF;
   led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, badge_system_data()->backlight);
   save_settings();

   returnToMenus();
}

const struct menu_t backlightList_m[] = {
//    {"       ", 0|VERT_ITEM, FUNCTION, { .func = backlight_cb} }, oh support... why is my screen black?
    {"      -",  16|VERT_ITEM, FUNCTION, { .func = backlight_cb} },
    {"     --",  32|VERT_ITEM, FUNCTION, { .func = backlight_cb} },
    {"    ---",  64|VERT_ITEM, FUNCTION, { .func = backlight_cb} },
    {"   ----", 128|VERT_ITEM, FUNCTION, { .func = backlight_cb} },
    {"  -----", 192|VERT_ITEM, FUNCTION, { .func = backlight_cb} },
    {" ------", 224|VERT_ITEM, FUNCTION, { .func = backlight_cb} },
    {"-------", 255|VERT_ITEM, FUNCTION, { .func = backlight_cb} },

    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

struct menu_t backlight_m[] = {
    {"-------",   VERT_ITEM,     MENU, {backlightList_m} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

#if 0
/*
   rotate screen
*/
void rotate_cb(__attribute__((unused)) struct menu_t *h) {

    badge_system_data()->display_rotated = !badge_system_data()->display_rotated;

    display_set_rotation(badge_system_data()->display_rotated);
    save_settings();

    returnToMenus();
};
#endif

static void invert_cb(__attribute__((unused)) struct menu_t *h) {

    SYSTEM_DATA *system_data = badge_system_data();
    bool inverted = !system_data->display_inverted;
    system_data->display_inverted = inverted;

	if (inverted) {
        display_set_display_mode_inverted();
    } else {
        display_set_display_mode_noninverted();
    }

    save_settings();
	returnToMenus();
}

const struct menu_t rotate_m[] = {
    {"Inverted",   1|VERT_ITEM, FUNCTION, { .func = invert_cb} },
    {"Back",      VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};



/*
    LED brightness
*/
void LEDlight_cb(__attribute__((unused)) struct menu_t *h) {
    struct menu_t *dstMenu, *selectedMenu;

    dstMenu = getSelectedMenuStack(1);
    selectedMenu = getSelectedMenu();

    strcpy(dstMenu->name, selectedMenu->name);

    badge_system_data()->ledBrightness = selectedMenu->attrib & 0xFF;
    led_pwm_set_scale(badge_system_data()->ledBrightness);

    save_settings();
    returnToMenus();
}


const struct menu_t LEDlightList_m[] = {
//    {"       ", 7|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {"      -", 0|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {"     --", 30|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {"    ---", 75|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {"   ----", 100|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {"  -----", 150|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {" ------", 200|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {"-------", 255|VERT_ITEM, FUNCTION, { .func = LEDlight_cb} },
    {"Back", VERT_ITEM|LAST_ITEM| DEFAULT_ITEM, BACK, {NULL} },
};

struct menu_t LEDlight_m[] = {
    {"--------",   VERT_ITEM,     MENU, {LEDlightList_m} },
    {"Back", VERT_ITEM|LAST_ITEM| DEFAULT_ITEM, BACK, {NULL} },
};

void buzzer_config_cb(__attribute__((unused)) struct menu_t *menu)
{
    struct menu_t *dstMenu, *selectedMenu;

    dstMenu = getSelectedMenuStack(1); /* parent menu */
    selectedMenu = getSelectedMenu();

    strcpy(dstMenu->name, selectedMenu->name);

    badge_system_data()->mute = selectedMenu->attrib & 0x1; /* low order bits of attrib can store values */

    save_settings();
    returnToMenus();
}

const struct menu_t buzzer_config_m[] = {
    {"Audio: On",   0|VERT_ITEM,     FUNCTION, { .func = buzzer_config_cb} },
    {"Audio: Off",  1|VERT_ITEM,     FUNCTION, { .func = buzzer_config_cb} },
    {"Back", VERT_ITEM|LAST_ITEM| DEFAULT_ITEM, BACK, {NULL} },
};

/*
  not const menu_t ...  because the config status is stored in buzzer_m[0].name[]
*/
struct menu_t buzzer_m[] = {
    {"Audio: On",   VERT_ITEM,     MENU, {buzzer_config_m} },
    {"Back", VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};

void screen_save_lock_cb(__attribute__((unused)) struct menu_t *h) {
    struct menu_t *selectedMenu;
    selectedMenu = getSelectedMenu();
    badge_system_data()->screensaver_disabled = selectedMenu->attrib & 0x1FF;
    returnToMenus(); 
}

void screen_save_invert_cb(__attribute__((unused)) struct menu_t *menu)
{
    SYSTEM_DATA *system_data = badge_system_data();
    system_data->screensaver_inverted = !system_data->screensaver_inverted;
    returnToMenus();
}

const struct menu_t screen_lock_m[] = {
    {"ON",       0|VERT_ITEM, FUNCTION, { .func = screen_save_lock_cb} },
    {"OFF",      1|VERT_ITEM, FUNCTION, { .func = screen_save_lock_cb} },
    {"INVERT", VERT_ITEM, FUNCTION, { .func = screen_save_invert_cb} },
    {"TEST", VERT_ITEM, FUNCTION, { .func = test_screensavers_cb, }, },
    {"Back",   VERT_ITEM|LAST_ITEM|DEFAULT_ITEM, BACK, {NULL} },
};


void setup_settings_menus(void) {
    SYSTEM_DATA *systemData = badge_system_data();

    // Set up backlight brightness menu. Match the current setting to a menu option.
    for (size_t i = 0; i < (sizeof(backlightList_m) / sizeof(backlightList_m[0])-1); i++) {
        if (systemData->backlight <= (backlightList_m[i].attrib & 0xFF)) {
            memcpy(backlight_m[0].name, backlightList_m[i].name, sizeof(backlight_m[0].name));
            break;
        }
    }

    // Set up LED brightness menu in a similar way.
    for (size_t i = 0; i < (sizeof(LEDlightList_m) / sizeof(LEDlightList_m[0])-1); i++) {
        if (systemData->ledBrightness <= (LEDlightList_m[i].attrib & 0xFF)) {
            memcpy(LEDlight_m[0].name, LEDlightList_m[i].name, sizeof(LEDlight_m[0].name));
            break;
        }
    }

    // Set up buzzer on/off menu
    if (systemData->mute) {
        memcpy(buzzer_m[0].name, buzzer_config_m[1].name, sizeof(buzzer_m[0].name));
    } else {
        memcpy(buzzer_m[0].name, buzzer_config_m[0].name, sizeof(buzzer_m[0].name));
    }
}
