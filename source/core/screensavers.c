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
    FbMove(120 - 20, 5);
    FbColor(RED);
    FbRotWriteLine(drag_hack);

    int i = 0;
    for(i=0; i<(8 - popup_time%8); i++){
        // FbMove(17, 35+ (i*10));
	FbMove(128 - (i * 10) - 35, 17);
        FbRotWriteLine(drag_hack_num);
    }
    FbSwapBuffers();
}

static int random_num(int n)
{
	unsigned int rnd;
	random_insecure_bytes((uint8_t*)&rnd, sizeof(unsigned int));
	return (rnd % n);
}

void stupid_rects(void)
{
	static const int colors[] = { RED, YELLOW, GREEN, CYAN, WHITE, BLUE, MAGENTA };
	int x1, y1, x2, y2;
	int color = random_num(sizeof(colors) / sizeof(colors[0]));

	animation_count++;
	if (animation_count == 1 || (animation_count % 5) == 0) {
		x1 = random_num(LCD_XSIZE);
		y1 = random_num(LCD_YSIZE);
		x2 = random_num(LCD_XSIZE);
		y2 = random_num(LCD_YSIZE);
		if (x1 > x2) {
			int t = x1;
			x1 = x2;
			x2 = t;
		}
		if (y1 > y2) {
			int t = y1;
			y1 = y2;
			y2 = t;
		}
		switch (animation_count) {
		case 1:
			led_pwm_enable(BADGE_LED_RGB_RED, 255);
			led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
			led_pwm_disable(BADGE_LED_RGB_BLUE);
			break;
		case 5:
			led_pwm_disable(BADGE_LED_RGB_RED);
			led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
			led_pwm_disable(BADGE_LED_RGB_BLUE);
			break;
		case 10:
			led_pwm_disable(BADGE_LED_RGB_RED);
			led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
			led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
			break;
		case 15:
			led_pwm_enable(BADGE_LED_RGB_RED, 255);
			led_pwm_enable(BADGE_LED_RGB_GREEN, 255);
			led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
			break;
		case 20:
			led_pwm_disable(BADGE_LED_RGB_RED);
			led_pwm_disable(BADGE_LED_RGB_GREEN);
			led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
			break;
		case 25:
			led_pwm_enable(BADGE_LED_RGB_RED, 255/2);
			led_pwm_disable(BADGE_LED_RGB_GREEN);
			led_pwm_enable(BADGE_LED_RGB_BLUE, 255);
			break;
		}
		FbColor(colors[color]);
		FbMove(x1, y1);
		FbFilledRectangle(x2 - x1 + 1, y2 - y1 + 1);
		FbPushBuffer();
		if (animation_count > 25)
			animation_count = 0;
	}
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
    FbColor(WHITE);
    FbBackgroundColor(BLUE);
    FbClear();
    FbMove(0,0);

    FbMove(25, 25);
    FbFilledRectangle(10, 75);

    FbMove(55, 4);
    FbRotWriteLine(bs2);

    if(popup_time < 40){
        FbMove(75, 4);
        FbRotWriteLine(bs3);

        FbMove(85, 17);
        FbRotWriteLine(bs4);
    }

    FbMove(26, 27);
    FbRotWriteLine(bs1);
    FbSwapBuffers();
    FbColor(WHITE);
    FbBackground(BLACK);
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
    FbMove(LCD_XSIZE - 8 - 4, 14);
    FbWriteLine(badgetips_header);


    FbColor(YELLOW);
    FbMove(LCD_XSIZE - 20 - 20 , 4);
    switch(tipnum){
        case 0:
            FbRotWriteString("Dont lick the\nbadge");
            break;
        case 1:
            FbRotWriteString("These are land\ndwelling badges.\nAvoid water");
            break;
        case 2:
            FbRotWriteString("Known to the\nstate of cancer to\ncause California");
            break;
        case 3:
            FbRotWriteString("Wash your hands\nnext time.");
            break;
        case 4:
            FbRotWriteString("Say hi to\nthe creators\nover at hackrva");
            break;
        case 5:
            FbRotWriteString("Have another\nbeer.");
            break;
        case 6:
            FbRotWriteString("This badge is\nzombie load\nenabled");
            break;
        case 7:
            FbRotWriteString("This badge is\nloyal to Mark\nZuckyZuck");
            break;
        case 8:
            FbRotWriteString("checkout the\nCTF!");
            break;
        case 9:
            FbRotWriteString("Its impolite\nto start at\nother peoples\nbadges.");
            break;
        case 10:
            FbRotWriteString("Every badge is\ndifferent. Find\nthe bugs in\nyours!");
            break;
        case 11:
            FbRotWriteString("Badges can be\nvery social.\nTry playing\nIR games");
            break;
        case 12:
            FbRotWriteString("Its a thin line\nbetween a badge\nand an bodge");
            break;
        case 13:
            FbRotWriteString("If you cant\nread this your\nbadge is broken");
            break;
        case 14:
            FbRotWriteString("Youll find the\nsource code\nonline after\nthe conference.");
            break;
        case 15:
            FbRotWriteString("The badge is\nnot a touch\nscreen");
            break;
        case 16:
            FbRotWriteString("Badges need\nlove. Pet\nyour badge\nregularly");
            break;
        case 17:
            FbRotWriteString("These are\nartisanal\nbadges");
            break;
        case 18:
        default:
            FbRotWriteString("Badges are\nhand crafted\nat hackrva");
            break;
    }

    FbSwapBuffers();
}
