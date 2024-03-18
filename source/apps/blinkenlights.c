#include "colors.h"
#include "menu.h"
#include "ir.h"
#include "blinkenlights.h"
#include "button.h"
#include "led_pwm.h"

#define BL_INCR_AMNT 5

// If ordering changed, make sure indices still work
// in the populate function
enum {
    BLINKENLIGHTS_RED = 0,
    BLINKENLIGHTS_BLUE,
    BLINKENLIGHTS_GREEN,
    BLINKENLIGHTS_CLEAR,
    BLINKENLIGHTS_SPACE,
    BLINKENLIGHTS_MODE,
    BLINKENLIGHTS_GO,
    BLINKEN_LIGHTS_EXIT
};

enum
{
    INIT,
    SHOW_MENU,
    CONFIG_RED,
    CONFIG_BLUE,
    CONFIG_GREEN,
    RUN_BLINKENLIGHTS
};

enum
{
    LOCAL_ONLY,
    BCAST_ONLY,
    LOCAL_AND_BCAST
};

static char bl_state = INIT;
static char bl_mode = LOCAL_ONLY;

static void set_red(__attribute__((unused)) struct menu_t *m)
{
    bl_state = CONFIG_RED;
}

static void set_blue(__attribute__((unused)) struct menu_t *m)
{
    bl_state = CONFIG_BLUE;
}

static void set_green(__attribute__((unused)) struct menu_t *m)
{
    bl_state = CONFIG_GREEN;
}

static unsigned char bl_red = 50, bl_green = 40, bl_blue = 0;

static struct menu_t blinkenlights_config_m[];
static void bl_populate_menu(__attribute__((unused)) struct menu_t *m)
{
    blinkenlights_config_m[0].name[5] = '0' + (bl_red/100) % 10;
    blinkenlights_config_m[0].name[6] = '0' + (bl_red/10) % 10;
    blinkenlights_config_m[0].name[7] = '0' + bl_red % 10;
    blinkenlights_config_m[0].name[8] = 0;

    blinkenlights_config_m[1].name[6] = '0' + (bl_blue/100) % 10;
    blinkenlights_config_m[1].name[7] = '0' + (bl_blue/10) % 10;
    blinkenlights_config_m[1].name[8] = '0' + bl_blue % 10;
    blinkenlights_config_m[1].name[9] = 0;

    blinkenlights_config_m[2].name[7] = '0' + (bl_green/100) % 10;
    blinkenlights_config_m[2].name[8] = '0' + (bl_green/10) % 10;
    blinkenlights_config_m[2].name[9] = '0' + bl_green % 10;
    blinkenlights_config_m[2].name[10] = 0;
    
    if(bl_mode == LOCAL_ONLY)
    {
        blinkenlights_config_m[5].name[6] = 'l';
        blinkenlights_config_m[5].name[7] = 'o';
        blinkenlights_config_m[5].name[8] = 'c';
        blinkenlights_config_m[5].name[9] = 'a';
        blinkenlights_config_m[5].name[10] = 'l';
        blinkenlights_config_m[5].name[11] = 0;
    }
    else if(bl_mode == BCAST_ONLY)
    {
        blinkenlights_config_m[5].name[6] = 'b';
        blinkenlights_config_m[5].name[7] = 'c';
        blinkenlights_config_m[5].name[8] = 'a';
        blinkenlights_config_m[5].name[9] = 's';
        blinkenlights_config_m[5].name[10] = 't';
        blinkenlights_config_m[5].name[11] = 0;
    }
    else if(bl_mode == LOCAL_AND_BCAST)
    {
        blinkenlights_config_m[5].name[6] = 'b';
        blinkenlights_config_m[5].name[7] = 'o';
        blinkenlights_config_m[5].name[8] = 't';
        blinkenlights_config_m[5].name[9] = 'h';
        blinkenlights_config_m[5].name[10] = 0;
    }
}

static void bl_clear_colors(__attribute__((unused)) struct menu_t *m)
{
    bl_red = 0;
    bl_green = 0;
    bl_blue = 0;
    bl_populate_menu(NULL);
}

static void set_bl_mode(__attribute__((unused)) struct menu_t *m)
{
    if(bl_mode == LOCAL_ONLY || bl_mode == BCAST_ONLY)
        bl_mode++;
    else
        bl_mode = LOCAL_ONLY;

    bl_populate_menu(NULL);
}

static void set_local_leds(void)
{
    led_pwm_enable(BADGE_LED_RGB_RED, bl_red * 255 / 100);
    led_pwm_enable(BADGE_LED_RGB_GREEN, bl_green * 255 / 100);
    led_pwm_enable(BADGE_LED_RGB_BLUE, bl_blue * 255 / 100);
}

static void set_bl_go(__attribute__((unused)) struct menu_t *m)
{
    if(bl_mode == BCAST_ONLY || bl_mode == LOCAL_AND_BCAST)
    {
        uint16_t data = PACKRGB( bl_red, bl_green, bl_blue);
        uint8_t byte_data[2];
        byte_data[0] = data >> 8;
        byte_data[1] = data;

        IR_DATA ir_packet = {0};
        ir_packet.app_address = IR_LED;
        ir_packet.recipient_address = IR_BADGE_ID_BROADCAST;
        ir_packet.data = byte_data;
        ir_packet.data_length = 2;
        ir_send_complete_message(&ir_packet);
    }

    if(bl_mode == LOCAL_ONLY || bl_mode == LOCAL_AND_BCAST)
    {
        set_local_leds();
    }
}

static void set_bl_exit(__attribute__((unused)) struct menu_t *m)
{
    bl_state = INIT;
    returnToMenus();
}

static struct menu_t blinkenlights_config_m[] = {
    {"Red: ", VERT_ITEM, FUNCTION, { .func = set_red}},
    {"Blue: ", VERT_ITEM, FUNCTION, { .func = set_blue}},
    {"Green: ", VERT_ITEM, FUNCTION, { .func = set_green}},
    {"--CLEAR--", VERT_ITEM, FUNCTION, { .func = bl_clear_colors} },
    {"", VERT_ITEM|SKIP_ITEM, TEXT, {0}},
    {"Mode: ", VERT_ITEM, FUNCTION, { .func = set_bl_mode} },
    {"Go!!", VERT_ITEM|DEFAULT_ITEM, FUNCTION, { .func = set_bl_go} },
    {"Exit", VERT_ITEM|LAST_ITEM, FUNCTION, { .func =set_bl_exit}},
};

void blinkenlights_cb(__attribute__((unused)) struct menu_t *m)
{
    int down_latches = button_down_latches();
    switch(bl_state)
    {
        case INIT:
            bl_populate_menu(NULL);
            bl_state++;
            break;
        case SHOW_MENU:
            genericMenu((struct menu_t *)blinkenlights_config_m, MAIN_MENU_STYLE, down_latches);
            break;
        case CONFIG_RED:;
#if BADGE_HAS_ROTARY_SWITCHES
            if(BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
            {
                led_pwm_enable(BADGE_LED_RGB_RED, 0);
                led_pwm_enable(BADGE_LED_RGB_GREEN, 0);
                led_pwm_enable(BADGE_LED_RGB_BLUE, 0);
                bl_state = SHOW_MENU;
            }
            else 
#endif
            if(BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
            {
                bl_red += BL_INCR_AMNT;
                if(bl_red > 100)
                    bl_red = 100;
                
                set_local_leds();
                bl_populate_menu(NULL);
                display_menu(blinkenlights_config_m, &blinkenlights_config_m[BLINKENLIGHTS_RED], MAIN_MENU_STYLE);
                
            }
            else if(BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
            {
                if(bl_red  > BL_INCR_AMNT)
                    bl_red -= BL_INCR_AMNT;
                else
                    bl_red = 0;

                set_local_leds();
                bl_populate_menu(NULL);
                display_menu(blinkenlights_config_m, &blinkenlights_config_m[BLINKENLIGHTS_RED], MAIN_MENU_STYLE);
            }
            break;
        case CONFIG_GREEN:
#if BADGE_HAS_ROTARY_SWITCHES
            if(BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
            {
                led_pwm_enable(BADGE_LED_RGB_RED, 0);
                led_pwm_enable(BADGE_LED_RGB_GREEN, 0);
                led_pwm_enable(BADGE_LED_RGB_BLUE, 0);
                bl_state = SHOW_MENU;
            }
            else
#endif
            if(BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
            {
                bl_green += BL_INCR_AMNT;
                if(bl_green > 100)
                    bl_green = 100;

                set_local_leds();
                bl_populate_menu(NULL);
                display_menu(blinkenlights_config_m, &blinkenlights_config_m[BLINKENLIGHTS_GREEN], MAIN_MENU_STYLE);

            }
            else if(BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
            {
                if(bl_green > BL_INCR_AMNT)
                    bl_green -= BL_INCR_AMNT;
                else
                    bl_green = 0;

                set_local_leds();
                bl_populate_menu(NULL);
                display_menu(blinkenlights_config_m, &blinkenlights_config_m[BLINKENLIGHTS_GREEN], MAIN_MENU_STYLE);
            }
            break;
        case CONFIG_BLUE:
#if BADGE_HAS_ROTARY_SWITCHES
            if(BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
            {
                led_pwm_enable(BADGE_LED_RGB_RED, 0);
                led_pwm_enable(BADGE_LED_RGB_GREEN, 0);
                led_pwm_enable(BADGE_LED_RGB_BLUE, 0);
                bl_state = SHOW_MENU;
            }
            else
#endif
            if(BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
            {
                bl_blue += BL_INCR_AMNT;
                if(bl_blue > 100)
                    bl_blue = 100;

                set_local_leds();
                bl_populate_menu(NULL);
                display_menu(blinkenlights_config_m, &blinkenlights_config_m[BLINKENLIGHTS_BLUE], MAIN_MENU_STYLE);

            }
            else if(BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
            {
                if(bl_blue  > BL_INCR_AMNT)
                    bl_blue -= BL_INCR_AMNT;
                else
                    bl_blue = 0;

                set_local_leds();
                bl_populate_menu(NULL);
                display_menu(blinkenlights_config_m, &blinkenlights_config_m[BLINKENLIGHTS_BLUE], MAIN_MENU_STYLE);
            }
            break;
    }
}
