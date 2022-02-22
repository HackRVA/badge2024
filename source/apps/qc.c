#include "button.h"
#include "ir.h"
#include "colors.h"
#include "framebuffer.h"
#include "menu.h"
#include "audio_output.h"
#include "led_pwm.h"

// TODO write an IR callback that sets this variable!
unsigned char QC_IR;

enum {
    INIT,
    RUN
};

static void led(int red, int green, int blue) {
    led_pwm_enable(BADGE_LED_RGB_RED, red*255/100);
    led_pwm_enable(BADGE_LED_RGB_GREEN, green*255/100);
    led_pwm_enable(BADGE_LED_RGB_BLUE, blue*255/100);
}

#define LED_LVL 50
void QC_cb()
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

    int down_latches = button_down_latches();

    if (button_poll(BADGE_BUTTON_SW)) {
        button_hold_count ++;
    } else {
        button_hold_count = 0;
    }

    switch(QC_state)
    {
//        if(pinged){
//            setNote(77, 1024);
//            FbClear();
//            FbMove(40, 50);
//            FbColor(GREEN);
//            //FbFilledRectangle(20, 20);
//            FbWriteLine('IR RECV');
//            setNote(50, 2048);
//            led(100, 0, 100);
//            pinged = 0;
//            redraw = 1;
//        }
        case INIT:
            FbTransparentIndex(0);
            FbColor(GREEN);
            FbClear();

            FbMove(45, 45);
            FbWriteLine("QC!");
            FbMove(15, 55);
            FbWriteLine("Do things");
            FbSwapBuffers();
            led(0, 30, 0);
            QC_IR = 0;
            QC_state++;
            redraw = 1;
            break;

        case RUN:
            // Received a QC ping
            if(QC_IR == 1){
                audio_set_note(80, 4096);
                FbMove(10, 40);
                FbColor(GREEN);
                //FbFilledRectangle(20, 20);
                FbWriteLine("I was pinged");
                led(100, 0, 100);
                QC_IR = 0;
                data = 2;
                ir_send_complete_message(&ir_packet);
                redraw = 1;
            }
                // Received a QC ping
            else if(QC_IR == 2){
                audio_set_note(60, 2048);
                FbMove(10, 50);
                FbColor(YELLOW);
                //FbFilledRectangle(20, 20);
                //FbWriteLine('IR RECV');
                FbWriteLine("ping response");
                led(100, 100, 0);
                QC_IR = 0;
                redraw = 1;
            }

            if(down_latches & (1<<BADGE_BUTTON_SW)){
                data = 1;
                ir_send_complete_message(&ir_packet);
                audio_set_note(50, 1024);
                FbMove(16, 16);
                FbWriteLine("BTN");
                led(LED_LVL, 0, LED_LVL);
                //print_to_com1("DOWN\n\r");
                redraw = 1;
            }

            if(down_latches & (1<<BADGE_BUTTON_DOWN)){
                FbMove(16, 16);
                FbWriteLine("DOWN");
                audio_set_note(55, 1024);
                led(0, LED_LVL, 0);
                //print_to_com1("DOWN\n\r");
                redraw = 1;
            }

            if(button_hold_count > 200){
                FbMove(16, 26);
                FbWriteLine("EXITING");
                FbSwapBuffers();
                led(0,0,0);
                QC_state = 0;
                returnToMenus();
                return;
            }

            if(down_latches & (1<<BADGE_BUTTON_UP)){
                FbMove(16, 16);
                FbWriteLine("UP");
                audio_set_note(60, 1024);
                led(LED_LVL, LED_LVL, LED_LVL);
                //print_to_com1("UP\n\r");
                redraw = 1;
            }

            if(down_latches & (1<<BADGE_BUTTON_LEFT)){
                FbMove(16, 16);
                FbWriteLine("LEFT");
                audio_set_note(45, 1024);
                led(LED_LVL, 0, 0);
                //print_to_com1("LEFT\n\r");
                redraw = 1;
            }

            if(down_latches & (1<<BADGE_BUTTON_RIGHT)){
                FbMove(16, 16);
                FbWriteLine("RIGHT");
                led(0, 0, LED_LVL);
                audio_set_note(20, 1024);
                //print_to_com1("RIGHT");
                redraw = 1;
            }

            if(redraw){
                redraw = 0;
                FbSwapBuffers();
            }
    }
}