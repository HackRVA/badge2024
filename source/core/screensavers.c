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

static unsigned short animation_count = 0;
void screensaver_set_animation_count(unsigned short count)
{
	animation_count = count;
}

#define IB1 1
#define IB2 2
#define IB5 16
#define IB18 131072

#define MASK (IB1+IB2+IB5)

static unsigned int irbit2(unsigned int iseed) {
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
    if(!animation_count){
        uint8_t random;
        random_insecure_bytes(&random, sizeof(int8_t));

        imgnum = random % 3;
        animation_count++;
    }

    switch(imgnum){
        case 0:
            FbMove(0,0);
            FbImage(&assetList[HACKRVA4], 0);
            break;

        case 1:
            FbMove(0,20);
            FbImage2bit(&assetList[RVASEC2016], 0);
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

    int i = 0;
    for(i=0; i<(8 - popup_time%8); i++){
        FbMove(17, 35+ (i*10));
        FbWriteLine(drag_hack_num);
    }

    FbSwapBuffers();

}

void stupid_rects(){
    unsigned int rnd;
    random_insecure_bytes((uint8_t*)&rnd, sizeof(unsigned int));
    animation_count++;

    if(animation_count==1){
        FbColor(YELLOW);
        FbMove(rnd%20+rnd%20,rnd%70+rnd%30);

        FbFilledRectangle(rnd%80, rnd%20);
        led_pwm_enable(BADGE_LED_RGB_RED, 255);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_disable(BADGE_LED_RGB_BLUE);
    }
    else if(animation_count == 5){
        FbColor(GREEN);
        FbMove(rnd%60+rnd%60,rnd%55+rnd%10);

        FbFilledRectangle(rnd%10, rnd%50);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_disable(BADGE_LED_RGB_BLUE);
    }
    else if(animation_count == 10){
        FbColor(CYAN);
        FbMove(rnd%70+rnd%45,rnd%45+rnd%33);

        FbFilledRectangle(rnd%25, rnd%30);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(animation_count == 15){

        FbColor(WHITE);
        FbMove(rnd%30+rnd%10,rnd%30+rnd%30);

        FbFilledRectangle(rnd%70, rnd%20);
        led_pwm_enable(BADGE_LED_RGB_RED, 255);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(animation_count == 20){

        FbColor(BLUE);
        FbMove(rnd%50+rnd%30,rnd%10+rnd%15);

        FbFilledRectangle(rnd%30, rnd%50);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_disable(BADGE_LED_RGB_GREEN);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(animation_count == 25){

        FbColor(MAGENTA);
        FbMove(rnd%33+rnd%47,rnd%65+rnd%33);

        FbFilledRectangle(rnd%40, rnd%30);
        led_pwm_enable(BADGE_LED_RGB_RED, 255/2);
        led_pwm_disable(BADGE_LED_RGB_GREEN);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(animation_count > 25)
        animation_count = 0;

    FbPushBuffer();
}

#define TUNNEL_COLOR CYAN
void carzy_tunnel_animator(){
    animation_count++;

    if(!animation_count){
        FbColor(TUNNEL_COLOR);
        FbMove(65,65);

        FbRectangle(2, 2);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(animation_count == 5){
        FbColor(TUNNEL_COLOR);
        FbMove(64,64);

        FbRectangle(4, 4);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 5*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 5*255/100);
    }
    else if(animation_count == 10){
        FbColor(TUNNEL_COLOR);
        FbMove(62,62);

        FbRectangle(8, 8);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 20*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 20*255/100);
    }
    else if(animation_count == 15){
        FbColor(TUNNEL_COLOR);
        FbMove(58,58);

        FbRectangle(16, 16);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 45*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 45*255/100);
    }
    else if(animation_count == 20){
        FbColor(TUNNEL_COLOR);
        FbMove(50,50);

        FbRectangle(32, 32);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 75*255/100);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 75*255/100);
    }
    else if(animation_count == 25){
        FbColor(TUNNEL_COLOR);
        FbMove(34,34);

        FbRectangle(64, 64);
        led_pwm_disable(BADGE_LED_RGB_RED);
        led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
        led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
    }
    else if(animation_count > 30)
        animation_count = 0 - 1;

    FbSwapBuffers();
}

void dotty(){

    unsigned int rnd;
    random_insecure_bytes((uint8_t*)&rnd, sizeof(unsigned int));
    if(animation_count == 0){
        FbClear();
    }

    //if(animation_count%5){
    unsigned char i = 0;
    for(i = 0; i < 200; i++)
    {
        FbColor(BLUE << (animation_count>>4));
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
    animation_count += 2;

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
            FbColor((i+(animation_count/15))%2 ? WHITE: RED);
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
    animation_count++;
    FbSwapBuffers();
    led_pwm_enable(BADGE_LED_RGB_BLUE, 255);

}

static void smiley_eye(int x, int y)
{
	/* Whites of the eye */
	FbColor(WHITE);
	FbMove(x, y);
	FbFilledRectangle(35, 20);

	/* Iris */
	FbColor(BLACK);
	if(popup_time > 40)
		FbMove(x + 17, y + 10);
	else if(popup_time > 30)
		FbMove(x + 25, y + 15);
	else if(popup_time > 20)
		FbMove(x + 10, y + 5);
	else
		FbMove(x + 17, y + 10);
	FbFilledRectangle(7, 7);
}

static void smiley_mouth(int x, int y)
{
	FbColor(WHITE);
	FbMove(x, y);
	FbFilledRectangle(5, 80);
}

static void smiley_tongue(int x, int y)
{
	int dx;

	dx = popup_time > 20 ? 20 : popup_time;
	FbColor(RED);
	FbMove(x + 20 - dx, y);
	FbFilledRectangle(popup_time > 20 ? 20 : popup_time, 30);
	FbMove(x + 20 - dx - 5, y + 5);
	FbFilledRectangle(5, 20);
}

void smiley(void)
{
	int dx = popup_time > 20 ? 20 : popup_time;

	smiley_eye(90, 20);
	smiley_eye(90, 120);
	smiley_mouth(45, 40);
	smiley_tongue(30, 60 + dx);
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
    if(!animation_count){
        int random;
        random_insecure_bytes((uint8_t*)&random, sizeof(int));
        tipnum = random % 19;
        animation_count++;
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
