#include "colors.h"
#include "ir.h"
#include "assetList.h"
#include "button.h"
#include "framebuffer.h"
#include "screensavers.h"
#include "random.h"
#include "led_pwm.h"
#include "rtc.h"

extern unsigned short popup_time;
extern void render_screen_save_monsters(void);

unsigned short anim_cnt = 0;


#define IB1 1
#define IB2 2
#define IB5 16
#define IB18 131072

#define MASK (IB1+IB2+IB5)

unsigned static int irbit2(unsigned int iseed) {
    if (iseed & IB18){
        iseed = ((iseed ^ MASK) << 1) | IB1;
    }
    else{
        iseed <<= 1;
    }
    return iseed;
}

void disp_asset_saver(){
    static unsigned char imgnum = 0;
    if(!anim_cnt){
        uint8_t random;
        random_insecure_bytes(&random, sizeof(int8_t));

        imgnum = random % 3;
        anim_cnt++;
    }

    switch(imgnum){
        case 0:
            FbMove(0,0);
            FbImage(HACKRVA4, 0);
            break;

        case 1:
            FbMove(0,20);
            FbImage2bit(RVASEC2016, 0);
            break;

        case 2:
            render_screen_save_monsters();
            break;
    }

    FbSwapBuffers();
}

const char drag_hack[] = "Checkout HackRVA!";
const char drag_hack_num[] = "HackRVA.org";
void hack_the_dragon(){

    led_pwm_enable(BADGE_LED_RGB_RED, 255);
    FbMove(5, 20);
    FbColor(RED);
    FbWriteLine(drag_hack);

    unsigned int i = 0;
    for(i=0; i<(8 - popup_time%8); i++){
        FbMove(17, 35+ (i*10));
        FbWriteLine(drag_hack_num);
    }

    FbSwapBuffers();

}

void stupid_rects(){
    unsigned int rnd;
    random_insecure_bytes((uint8_t*)&rnd, sizeof(unsigned int));
    anim_cnt++;

    if(anim_cnt==1){
        FbColor(YELLOW);
        FbMove(rnd%20+rnd%20,rnd%70+rnd%30);

        FbFilledRectangle(rnd%80, rnd%20);
        led_pwm_enable(BADGE_LED_RGB_RED, 255);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_disable(BADGE_LED_RGB_BLUE);
    }
    else if(anim_cnt == 5){
        FbColor(GREEN);
        FbMove(rnd%60+rnd%60,rnd%55+rnd%10);

        FbFilledRectangle(rnd%10, rnd%50);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_disable(BADGE_LED_RGB_BLUE);
    }
    else if(anim_cnt == 10){
        FbColor(CYAN);
        FbMove(rnd%70+rnd%45,rnd%45+rnd%33);

        FbFilledRectangle(rnd%25, rnd%30);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(anim_cnt == 15){

        FbColor(WHITE);
        FbMove(rnd%30+rnd%10,rnd%30+rnd%30);

        FbFilledRectangle(rnd%70, rnd%20);
        led_pwm_enable(BADGE_LED_RGB_RED, 255);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(anim_cnt == 20){

        FbColor(BLUE);
        FbMove(rnd%50+rnd%30,rnd%10+rnd%15);

        FbFilledRectangle(rnd%30, rnd%50);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_disable(BADGE_LED_RGB_GREEN);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(anim_cnt == 25){

        FbColor(MAGENTA);
        FbMove(rnd%33+rnd%47,rnd%65+rnd%33);

        FbFilledRectangle(rnd%40, rnd%30);
        led_pwm_enable(BADGE_LED_RGB_RED, 255/2);
        led_pwm_disable(BADGE_LED_RGB_GREEN);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(anim_cnt > 25)
        anim_cnt = 0;

    FbPushBuffer();
}

#define TUNNEL_COLOR CYAN
void carzy_tunnel_animator(){
    //static unsigned short anim_cnt = 0;
    anim_cnt++;

    if(!anim_cnt){
        FbColor(TUNNEL_COLOR);
        FbMove(65,65);

        FbRectangle(2, 2);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(anim_cnt == 5){
        FbColor(TUNNEL_COLOR);
        FbMove(64,64);

        FbRectangle(4, 4);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 5*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 5*255/100);
    }
    else if(anim_cnt == 10){
        FbColor(TUNNEL_COLOR);
        FbMove(62,62);

        FbRectangle(8, 8);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 20*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 20*255/100);
    }
    else if(anim_cnt == 15){
        FbColor(TUNNEL_COLOR);
        FbMove(58,58);

        FbRectangle(16, 16);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 45*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 45*255/100);
    }
    else if(anim_cnt == 20){
        FbColor(TUNNEL_COLOR);
        FbMove(50,50);

        FbRectangle(32, 32);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 75*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 75*255/100);
    }
    else if(anim_cnt == 25){
        FbColor(TUNNEL_COLOR);
        FbMove(34,34);

        FbRectangle(64, 64);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(anim_cnt > 30)
        anim_cnt = 0 - 1;

    FbSwapBuffers();
}

void dotty(){

    unsigned int rnd;
    random_insecure_bytes((uint8_t*)&rnd, sizeof(unsigned int));
    if(anim_cnt == 0){
        FbClear();
    }

    //if(anim_cnt%5){
    unsigned char i = 0;
    for(i = 0; i < 200; i++)
    {
        FbColor(BLUE << (anim_cnt>>4));
        //FbPoint(rnd%130, irbit2(~timestamp)%130);
        //FbPoint(irbit2(get_rand_char(0, 132) + timestamp + i),
        //        irbit2(get_rand_char(0, 132) + ~timestamp + i));
        uint32_t random;
        random_insecure_bytes((uint8_t*)&random, sizeof(uint32_t));
        random %= 132;

        int timestamp = rtc_get_ms_since_boot();
        FbMove(irbit2(random + timestamp + i),
               irbit2(random + ~timestamp + i));
        FbFilledRectangle(3, 3);
    }

    FbPushBuffer();
    anim_cnt += 2;

}

const char president1[] = "HAL FOR";
const char president2[] = "president";
const char president3[] = "Badge for";
const char president4[] = "Vice President";
void for_president(){

    FbMove(22, 17);
    FbColor(WHITE);
    FbWriteLine(president1);

    FbMove(32, 35);
    FbWriteLine(president2);

    if(popup_time > 3){
        unsigned char i = 0;
        for(i=0; i<8; i++){
            FbColor((i+(anim_cnt/15))%2 ? WHITE: RED);
            FbMove(0, 50 + (i*10));
            FbFilledRectangle(132, 10);
        }
    }
    else{
        FbColor(WHITE);
        FbMove(32, 70);
        FbWriteLine(president3);

        FbMove(12, 80);
        FbWriteLine(president4);
    }
    anim_cnt++;
    FbSwapBuffers();
    led_pwm_enable(BADGE_LED_RGB_BLUE, 255);

}

void smiley(){

    FbColor(WHITE);

    FbMove(10, 10);
    FbFilledRectangle(20, 35);
    FbMove(100, 10);
    FbFilledRectangle(20, 35);
    FbMove(20, 80);
    FbFilledRectangle(80, 5);

    FbColor(BLACK);

    if(popup_time > 40)
        FbMove(15, 20);
    else if(popup_time > 30)
        FbMove(12, 15);
    else if(popup_time > 20)
        FbMove(19, 15);
    else
        FbMove(16, 25);

    FbFilledRectangle(7, 7);

    if(popup_time > 40)
        FbMove(105, 20);
    else if(popup_time > 30)
        FbMove(102, 15);
    else if(popup_time > 20)
        FbMove(109, 15);
    else
        FbMove(106, 25);

    FbFilledRectangle(7, 7);


    FbColor(RED);

    if(popup_time > 20){
        FbMove(60, 85);
        FbFilledRectangle(30, 20);
        FbMove(65, 105);
        FbFilledRectangle(20, 7);

    }
    else{
        FbMove(40+popup_time, 85);
        FbFilledRectangle(30, popup_time);
        FbMove(45+popup_time, 85+popup_time);
        FbFilledRectangle(20, 7);
    }


    FbSwapBuffers();
}

void matrix(){
    unsigned char w = 0, h = 0;
    FbColor(GREEN);
    for(w = 0; w <132; w+=10){
        for(h = 0; h < 132; h+=10){
            FbMove(w, h);
            //FbCharacter(lcd_font_map[irbit2(popup_time+w+h+timestamp)%42]);
        }
    }
    FbSwapBuffers();
}

const char bs1[] = "Badgedows";
const char bs2[] = "An error occured";
const char bs3[] = "Give up to";
const char bs4[] = "continue";
void bluescreen(){
    FbColor(BLUE);
    FbMove(0,0);
    FbFilledRectangle(132, 132);

    FbColor(WHITE);
    FbMove(25, 10);
    FbFilledRectangle(75, 10);

    FbMove(4, 40);
    FbWriteLine(bs2);

    if(popup_time < 40){
        FbMove(4, 60);
        FbWriteLine(bs3);

        FbMove(17, 70);
        FbWriteLine(bs4);
    }

    FbColor(BLUE);
    FbMove(27, 11);
    FbWriteLine(bs1);
    FbSwapBuffers();
}


const char badgetips_header[] = "--Badge Tip--";
//const unsigned char badgetip_more_you_know = 
void just_the_badge_tips(){
    static unsigned char tipnum = 0;
    if(!anim_cnt){
        int random;
        random_insecure_bytes((uint8_t*)&random, sizeof(int));
        tipnum = random % 19;
        anim_cnt++;
    }

    FbBackgroundColor(BLACK);
    FbColor(GREEN);
    FbMove(14, 4);
    FbWriteLine(badgetips_header);


    FbColor(YELLOW);
    switch(tipnum){
        case 0:
            FbMove(4, 20);
            FbWriteLine("Dont lick the");
            FbMove(4, 30);
            FbWriteLine("badge");
            break;
        case 1:
            FbMove(4, 20);
            FbWriteLine("These are land");
            FbMove(4, 30);
            FbWriteLine("dwelling badges.");
            FbMove(4, 40);
            FbWriteLine("Avoid water");
            break;
        case 2:
            FbMove(4, 20);
            FbWriteLine("Known to the");
            FbMove(4, 30);
            FbWriteLine("state of cancer to");
            FbMove(4, 40);
            FbWriteLine("cause california");
            break;
        case 3:
            FbMove(4, 20);
            FbWriteLine("Wash your hands");
            FbMove(4, 30);
            FbWriteLine("next time.");
            break;
        case 4:
            FbMove(4, 20);
            FbWriteLine("Say hi to");
            FbMove(4, 30);
            FbWriteLine("the creators");
            FbMove(4, 40);
            FbWriteLine("over at hackrva");
            break;
        case 5:
            FbMove(4, 20);
            FbWriteLine("Have another");
            FbMove(4, 30);
            FbWriteLine("beer.");
            break;
        case 6:
            FbMove(4, 20);
            FbWriteLine("This badge is");
            FbMove(4, 30);
            FbWriteLine("zombie load");
            FbMove(4, 40);
            FbWriteLine("Enabled");
            break;
        case 7:
            FbMove(4, 20);
            FbWriteLine("This badge is");
            FbMove(4, 30);
            FbWriteLine("loyal to Mark");
            FbMove(4, 40);
            FbWriteLine("ZuckyZuck");
            break;
        case 8:
            FbMove(4, 20);
            FbWriteLine("checkout the");
            FbMove(4, 30);
            FbWriteLine("   CTF!");
            break;
        case 9:
            FbMove(4, 20);
            FbWriteLine("Its impolite");
            FbMove(4, 30);
            FbWriteLine("to stare at");
            FbMove(4, 40);
            FbWriteLine("other peoples");
            FbMove(4, 50);
            FbWriteLine("badges.");
            break;
        case 10:
            FbMove(4, 20);
            FbWriteLine("Every badge is");
            FbMove(4, 30);
            FbWriteLine("different. Find");
            FbMove(4, 40);
            FbWriteLine("the bugs in");
            FbMove(4, 50);
            FbWriteLine("Yours!");
            break;
        case 11:
            FbMove(4, 20);
            FbWriteLine("Badges can be");
            FbMove(4, 30);
            FbWriteLine("very social.");
            FbMove(4, 40);
            FbWriteLine("Try playing");
            FbMove(4, 50);
            FbWriteLine("IR games.");
            break;
        case 12:
            FbMove(4, 20);
            FbWriteLine("Its a thin line");
            FbMove(4, 30);
            FbWriteLine("a badge and a");
            FbMove(4, 40);
            FbWriteLine("bodge.");
            break;
        case 13:
            FbMove(4, 20);
            FbWriteLine("If you cant");
            FbMove(4, 30);
            FbWriteLine("read this your");
            FbMove(4, 40);
            FbWriteLine("badge is broken");
            break;
        case 14:
            FbMove(4, 20);
            FbWriteLine("Youll find the");
            FbMove(4, 30);
            FbWriteLine("source code ");
            FbMove(4, 40);
            FbWriteLine("online after");
            FbMove(4, 50);
            FbWriteLine("the conference.");
            break;
        case 15:
            FbMove(4, 20);
            FbWriteLine("The badge is");
            FbMove(4, 30);
            FbWriteLine("not a touch");
            FbMove(4, 40);
            FbWriteLine("screen");
            break;
        case 16:
            FbMove(4, 20);
            FbWriteLine("Badges need");
            FbMove(4, 30);
            FbWriteLine("love. Pet");
            FbMove(4, 40);
            FbWriteLine("your badge");
            FbMove(4, 50);
            FbWriteLine("regularly.");
            break;
        case 17:
            FbMove(4, 20);
            FbWriteLine("These are");
            FbMove(4, 30);
            FbWriteLine("artisanal");
            FbMove(4, 40);
            FbWriteLine("badges.");
            break;
        case 18:
        default:
            FbMove(4, 20);
            FbWriteLine("Badges are");
            FbMove(4, 30);
            FbWriteLine("hand crafted");
            FbMove(4, 40);
            FbWriteLine("at hackrva");
            break;
    }

    FbSwapBuffers();
}
