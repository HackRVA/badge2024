#include "menu.h"
#include "ir.h"
#include "conductor.h"
#include "button.h"
#include "audio.h"

enum {
    CONDUCTOR_TOP = 0,
    CONDUCTOR_BOTTOM,
    CONDUCTOR_SPACE,
    CONDUCTOR_LEFT,
    CONDUCTOR_RIGHT,
    CONDUCTOR_SPACE2,
    CONDUCTOR_MODE,
    CONDUCTOR_GO,
    CONDUCTOR_EXIT
};

static void set_conductor_top_note(void);
static void set_exit(void);
static void set_conductor_bottom_note(void);
static void set_conductor_left_note(void);
static void set_conductor_right_note(void);
static void set_mode(void);
static void set_go(void);

static struct menu_t conductor_config_m[] = {
    {"Top: ", VERT_ITEM, FUNCTION, {(struct menu_t *)set_conductor_top_note} },
    {"Bottom: ", VERT_ITEM, FUNCTION, {(struct menu_t *)set_conductor_bottom_note} },
    {"", VERT_ITEM|SKIP_ITEM, TEXT, {0}},
    {"Left: ", VERT_ITEM, FUNCTION, {(struct menu_t *)set_conductor_left_note} },
    {"Right: ", VERT_ITEM, FUNCTION, {(struct menu_t *)set_conductor_right_note} },
    {"", VERT_ITEM|SKIP_ITEM, TEXT, {0}},
    {"Mode: ", VERT_ITEM, FUNCTION, {(struct menu_t *)set_mode} },
    {"Go!!", VERT_ITEM|DEFAULT_ITEM, FUNCTION, {(struct menu_t *)set_go} },
    {"Exit", VERT_ITEM|LAST_ITEM, FUNCTION, {(struct menu_t *) set_exit}},
};


static unsigned short top_note = 800;
static unsigned short bottom_note = 640;
static unsigned short left_note = 1280;
static unsigned short right_note = 5120;

enum
{
    INIT,
    SHOW_MENU,
    CONFIG_TOP,
    CONFIG_BOTTOM,
    CONFIG_LEFT,
    CONFIG_RIGHT,
    RUN_CONDUCTOR
};

enum
{
    LOCAL_ONLY,
    BCAST_ONLY,
    LOCAL_AND_BCAST
};

static char con_state = INIT;
static char con_mode = LOCAL_ONLY;

static void set_conductor_top_note(void)
{
    con_state = CONFIG_TOP;
}

static void set_conductor_bottom_note(void)
{
    con_state = CONFIG_BOTTOM;
}

static void set_conductor_left_note(void)
{
    con_state = CONFIG_LEFT;
}

static void set_conductor_right_note(void)
{
    con_state = CONFIG_RIGHT;
}

static void populate_menu(void);

static void set_mode(void)
{
    if(con_mode == LOCAL_ONLY || con_mode == BCAST_ONLY)
        con_mode++;
    else
        con_mode = LOCAL_ONLY;
    
    populate_menu();
}

static void set_go(void)
{
    con_state = RUN_CONDUCTOR;
}

static void set_exit(void)
{
    con_state = INIT;
    returnToMenus();
}

static void populate_menu(void)
{

    conductor_config_m[0].name[5] = '0' + (top_note/100) % 10;
    conductor_config_m[0].name[6] = '0' + (top_note/10) % 10;
    conductor_config_m[0].name[7] = '0' + top_note % 10;
    conductor_config_m[0].name[8] = 0;

    conductor_config_m[1].name[8] = '0' + (bottom_note/100) % 10;
    conductor_config_m[1].name[9] = '0' + (bottom_note/10) % 10;
    conductor_config_m[1].name[10] = '0' + bottom_note % 10;
    conductor_config_m[1].name[11] = 0;

    conductor_config_m[3].name[6] = '0' + (left_note/100) % 10;
    conductor_config_m[3].name[7] = '0' + (left_note/10) % 10;
    conductor_config_m[3].name[8] = '0' + left_note % 10;
    conductor_config_m[3].name[9] = 0;

    conductor_config_m[4].name[7] = '0' + (right_note/100) % 10;
    conductor_config_m[4].name[8] = '0' + (right_note/10) % 10;
    conductor_config_m[4].name[9] = '0' + right_note % 10;
    conductor_config_m[4].name[10] = 0;

    if(con_mode == LOCAL_ONLY)
    {
        conductor_config_m[6].name[6] = 'l';
        conductor_config_m[6].name[7] = 'o';
        conductor_config_m[6].name[8] = 'c';
        conductor_config_m[6].name[9] = 'a';
        conductor_config_m[6].name[10] = 'l';
        conductor_config_m[6].name[11] = 0;
    }
    else if(con_mode == BCAST_ONLY)
    {
        conductor_config_m[6].name[6] = 'b';
        conductor_config_m[6].name[7] = 'c';
        conductor_config_m[6].name[8] = 'a';
        conductor_config_m[6].name[9] = 's';
        conductor_config_m[6].name[10] = 't';
        conductor_config_m[6].name[11] = 0;
    }
    else if(con_mode == LOCAL_AND_BCAST)
    {
        conductor_config_m[6].name[6] = 'b';
        conductor_config_m[6].name[7] = 'o';
        conductor_config_m[6].name[8] = 't';
        conductor_config_m[6].name[9] = 'h';
        conductor_config_m[6].name[10] = 0;
    }
}


static void run_conductor(uint32_t down_latches)
{
    unsigned short freq=0;

    if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
    {
        //returnToMenus();
        con_state = SHOW_MENU;
    }

    if(button_poll(BADGE_BUTTON_UP))
    {
        freq = top_note;
    }

    if (button_poll(BADGE_BUTTON_DOWN))
    {
        freq = bottom_note;
    }

    if (button_poll(BADGE_BUTTON_LEFT))
    {
        freq = left_note;
    }

    if (button_poll(BADGE_BUTTON_RIGHT))
    {
        freq = right_note;
    }
    if(freq != 0)
    {
        if(con_mode == BCAST_ONLY || con_mode == LOCAL_AND_BCAST)
        {
            IR_DATA ir_packet = {0};
            ir_packet.recipient_address = IR_BADGE_ID_BROADCAST;
            ir_packet.data_length = 2;
            ir_packet.app_address = IR_LIVEAUDIO;
            uint16_t data = (((2048 >> 8) & 0xF) << 12) | (freq & 0x0FFF);
            uint8_t byte_data[2];
            byte_data[0] = data >> 8;
            byte_data[1] = data;
            ir_packet.data = byte_data;
            ir_send_complete_message(&ir_packet);
        }

        if(con_mode == LOCAL_AND_BCAST || con_mode == LOCAL_ONLY)
            audio_out_beep(freq, 400);
    }
}

void conductor_cb(void)
{
    int down_latches = button_down_latches();
    switch(con_state)
    {
        case INIT:
            populate_menu();
            con_state++;
            break;
        case SHOW_MENU:
            genericMenu(conductor_config_m, MAIN_MENU_STYLE, down_latches);
            break;
        case CONFIG_TOP:
            if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
                con_state = SHOW_MENU;
                //returnToMenus();
            else if(BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
            {
                top_note++;
                audio_out_beep(top_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_TOP], MAIN_MENU_STYLE);
            }
            else if(BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
            {
                top_note--;
                audio_out_beep(top_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_TOP], MAIN_MENU_STYLE);
            }
            break;
        case CONFIG_BOTTOM:
            if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
                con_state = SHOW_MENU;
                //returnToMenus();
            else if(BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
            {
                bottom_note++;
                audio_out_beep(bottom_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_BOTTOM], MAIN_MENU_STYLE);
            }
            else if(BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
            {
                bottom_note--;
                audio_out_beep(bottom_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_BOTTOM], MAIN_MENU_STYLE);
            }
            break;
        case CONFIG_LEFT:
            if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
                con_state = SHOW_MENU;
                //returnToMenus();
            else if(BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
            {
                left_note++;
                audio_out_beep(left_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_LEFT], MAIN_MENU_STYLE);
            }
            else if(BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
            {
                left_note--;
                audio_out_beep(left_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_LEFT], MAIN_MENU_STYLE);
            }
            break;
        case CONFIG_RIGHT:
            if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
                con_state = SHOW_MENU;
                //returnToMenus();
            else if(BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
            {
                right_note++;
                audio_out_beep(right_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_RIGHT], MAIN_MENU_STYLE);
            }
            else if(BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
            {
                right_note--;
                audio_out_beep(right_note, 400);
                populate_menu();
                display_menu(conductor_config_m, &conductor_config_m[CONDUCTOR_RIGHT], MAIN_MENU_STYLE);
            }
            break;
        case RUN_CONDUCTOR:
            run_conductor(down_latches);
            break;
    }

}
