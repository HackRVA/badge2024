#include "badge.h"
#include "colors.h"
#include "ir.h"
#include "assetList.h"
#include "button.h"
#include "framebuffer.h"
#include "screensavers.h"
#include "random.h"
#include "led_pwm.h"
#include "rtc.h"
#include "xorshift.h"
#include "trig.h"
#include "new_badge_monsters/new_badge_monsters.h"
#include <string.h>

extern unsigned short popup_time;

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

static int random_num(int n)
{
	static unsigned int xorshift_state = 0;
	if (xorshift_state == 0) {
		random_insecure_bytes((uint8_t*)&xorshift_state, sizeof(xorshift_state));
	}
	return xorshift(&xorshift_state) % n;
}

#define NHYPERSPACE_STARS 30
static struct hyperspace_star {
	int64_t lx, ly, x, y, vx, vy;
} hyperspace_star[NHYPERSPACE_STARS] = { 0 };

static void map_hyperspace_star(void (*func)(struct hyperspace_star *))
{
	for (int i = 0; i < NHYPERSPACE_STARS; i++)
		func(&hyperspace_star[i]);
}

static void initialize_hyperspace_star(struct hyperspace_star *s)
{
	do {
		s->x = random_num(LCD_XSIZE) * 256;
		s->y = random_num(LCD_YSIZE) * 256;
	} while (s->x == LCD_XSIZE / 2 && s->y == LCD_YSIZE / 2);

	s->lx = s->x;
	s->ly = s->y;
	s->vx = -((LCD_XSIZE / 2) - (s->x / 256));
	s->vy = -((LCD_YSIZE / 2) - (s->y / 256));
	if (s->vx == 0 && s->vy == 0) {
		s->vx = 1;
		s->vy = 1;
	}
	s->vx = s->vx * 20;
	s->vy = s->vy * 20;

}

static void draw_hyperspace_star(struct hyperspace_star *s)
{
	int x1, y1, x2, y2;
	x1 = s->x / 256;
	y1 = s->y / 256;
	x2 = s->lx / 256;
	y2 = s->ly / 256;
	FbLine(x1, y1, x2, y2);
}

static void move_hyperspace_star(struct hyperspace_star *s)
{
	s->lx = s->x;
	s->ly = s->y;
	s->x += s->vx;
	s->y += s->vy;
	s->vx = (s->vx * 300) / 256; /* multiply by approximately 1.2 */
	s->vy = (s->vy * 300) / 256; /* multiply by approximately 1.2 */
	int sx = s->x / 256;
	int sy = s->y / 256;
	if (sx < 0 || sx >= LCD_XSIZE || sy < 0 || sy >= LCD_YSIZE)
		initialize_hyperspace_star(s);
}

static void initialize_hyperspace_screen_saver(void)
{
	map_hyperspace_star(initialize_hyperspace_star);
}

static void draw_hyperspace_stars(void)
{
	static int angle = 0;
	int r, g, b, ra, ga, ba, color;
	angle += 1;
	if (angle >= 128)
		angle = 0;
	ra = angle;
	ga = ra + 42;
	if (ga >= 128)
		ga -= 128;
	ba = ga + 42;
	if (ba >= 128)
		ba -= 128;
	r = sine(ra);
	b = sine(ba);
	g = sine(ga);

	/* map color values from [-256-255] to [0-512] */
	r = (int) r + 256;
	g = (int) g + 256;
	b = (int) b + 256;

	r = r >> 4; /* top 5 bits */
	g = g >> 3; /* top 6 bits */
	b = b >> 4; /* top 5 bits */

	color = (r << 11) | (g << 5) | b;
	FbColor(color);
	map_hyperspace_star(draw_hyperspace_star);
}

static void move_hyperspace_stars(void)
{
	map_hyperspace_star(move_hyperspace_star);
}

static void draw_hyperspace_screensaver(void)
{
	static int initialized = 0;
	if (!initialized) {
		initialize_hyperspace_screen_saver();
		initialized = 1;
	}
	move_hyperspace_stars();
	draw_hyperspace_stars();
}

void hyperspace_screen_saver(void)
{
	draw_hyperspace_screensaver();
	FbSwapBuffers();
	animation_count++;
}

void holly_screensaver(void)
{
	int n;

	FbMove(0, 33);
	if ((animation_count % 30) < 15)
		n = 1;
	else
		n = 3;
	if ((animation_count % 50) < 5)
		n = 2;
	switch (n) {
		case 1:
			FbImage(&assetList[HOLLY01], 0);
			break;
		case 2:
			FbImage(&assetList[HOLLY02], 0);
			break;
		case 3:
			FbImage(&assetList[HOLLY03], 0);
			break;
	}
	FbSwapBuffers();
	animation_count++;
}

void nametag_screensaver(void)
{
	int len, y;
	const char *name = badge_system_data()->name;

	draw_hyperspace_screensaver();
	FbColor(YELLOW);
	FbBackgroundColor(BLACK);
	FbMove(LCD_XSIZE - 20, 10);
	FbRotWriteLine("HELLO MY NAME IS\n");
	len = strlen(name);
	y = ((LCD_YSIZE / 2) - (len * 9) / 2);
	FbMove(64, y);
	FbRotWriteLine(name);
	FbSwapBuffers();
}

void disp_asset_saver(void)
{
    static unsigned char imgnum = 0;
    if(!animation_count){
        uint8_t random;
        random_insecure_bytes(&random, sizeof(int8_t));

        imgnum = random % 4;
        animation_count++;
    }

    switch(imgnum){
        case 0:
            FbMove(0,0);
            FbImage(&assetList[RVASEC_LOGO], 0);
            break;

        case 1:
            FbMove(0,0);
            FbImage(&assetList[HACKRVA4], 0);
            break;

        case 2:
            FbMove(0,20);
            FbImage2bit(&assetList[RVASEC2016], 0);
            break;

        case 3:
            render_screen_save_monsters();
            break;
    }

    FbSwapBuffers();
}

const char drag_hack[] = "Checkout HackRVA!";
const char drag_hack_num[] = "HackRVA.org";
void hack_the_dragon(void)
{
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
void carzy_tunnel_animator(void)
{
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

void dotty(void)
{

    unsigned int rnd;
    random_insecure_bytes((uint8_t*)&rnd, sizeof(unsigned int));
    if(animation_count == 0){
        FbClear();
    }

    //if(animation_count%5){
    unsigned char i = 0;
    for(i = 0; i < 200; i++)
    {
        FbColor(BLUE << (animation_count / 16));
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

void for_president(void)
{
    FbMove(LCD_XSIZE - 8 - 17, 22);
    FbColor(WHITE);
    FbRotWriteString("HAL FOR\nPresident");

    if(popup_time > 3){
        unsigned char i = 0;
        for(i=0; i<8; i++){
            FbColor((i+(animation_count/15))%2 ? WHITE: RED);
            FbMove(LCD_XSIZE - (50 + (i*10)), 0);
            FbFilledRectangle(10, LCD_YSIZE);
        }
    } else {
        FbColor(WHITE);
        FbMove(LCD_XSIZE - 8 - 70, 32);
	FbRotWriteString("Badge for\nVice President");
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

void matrix(void)
{
#define NUM_MATRIX_DOODADS 5
	static int x[NUM_MATRIX_DOODADS], y[NUM_MATRIX_DOODADS];
	static int started = 0;
	if (!started) {
		FbColor(x11_light_green);
		FbBackgroundColor(BLACK);
		FbClear();
		for (int i = 0; i < NUM_MATRIX_DOODADS; i++) {
			x[i] = random_num(LCD_XSIZE / 8);
			y[i] = random_num(LCD_YSIZE / 8);
		}
		started = 1;
	}

	for (int i = 0; i < NUM_MATRIX_DOODADS; i++) {
		if (x[i] > 0)
			FbColor(x11_light_green);
		else
			FbColor(x11_dark_green);
		FbBackgroundColor(BLACK);
		unsigned char ch = random_num(63) + 'A';
		FbMove(x[i] * 8, y[i] * 8);
		FbRotCharacter(ch);
		if (random_num(1000) < 900) {
			int nx = x[i] + 1;
			if (nx * 8 <= LCD_XSIZE - 8) {
				FbColor(x11_dark_green);
				FbMove(nx * 8, y[i] * 8);
				ch = random_num(63) + 'A';
				FbRotCharacter(ch);
			}
		}
		x[i] -= 1;
		if (x[i] < 0) {
			x[i] = (LCD_XSIZE - 8) / 8;
			y[i] = random_num(LCD_YSIZE / 8);
		}
	}
	FbPushBuffer();
}

const char bs1[] = "Badgedows";
const char bs2[] = "An error occurred";
const char bs3[] = "Give up to";
const char bs4[] = "continue";
void bluescreen(void)
{
    FbColor(WHITE);
    FbBackgroundColor(BLUE);
    FbClear();
    FbMove(0,0);

    FbMove(25, 25);
    FbFilledRectangle(10, 75);

    FbMove(55, 4);
    FbRotWriteLine(bs2);

    if(popup_time < 40){
        FbMove(85, 4);
        FbRotWriteLine(bs3);

        FbMove(75, 17);
        FbRotWriteLine(bs4);
    }

    FbMove(26, 27);
    FbRotWriteLine(bs1);
    FbSwapBuffers();
    FbColor(WHITE);
    FbBackgroundColor(BLACK);
}

const char badgetips_header[] = "--Badge Tip--";
//const unsigned char badgetip_more_you_know = 
void just_the_badge_tips(void)
{
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
    FbRotWriteString(badgetips_header);

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

#define QIX_LINE_COUNT 20 
struct qixline {
	int x[2], y[2];
	uint16_t color;
};

static struct qix {
	struct qixline qline[QIX_LINE_COUNT];
	int vx[2], vy[2];
	int head, tail, initialized;
	int color_angle;
} the_qix = { 0 };

static void init_qix(struct qix *q)
{
	q->head = 0;
	q->tail = QIX_LINE_COUNT - 1;
	q->qline[0].x[0] = random_num(LCD_XSIZE);
	q->qline[0].y[0] = random_num(LCD_YSIZE);
	q->qline[0].x[1] = random_num(LCD_XSIZE);
	q->qline[0].y[1] = random_num(LCD_YSIZE);
	q->qline[0].color = WHITE;
	q->color_angle = 0;
	q->vx[0] = random_num(17) - 8;
	q->vy[0] = random_num(17) - 8;
	q->vx[1] = random_num(17) - 8;
	q->vy[1] = random_num(17) - 8;
	q->initialized = 1;
}

static void draw_qix(struct qix *q)
{
	int h = q->head;
	int t = q->tail;
	FbColor(q->qline[h].color);
	FbLine(q->qline[h].x[0], q->qline[h].y[0], q->qline[h].x[1], q->qline[h].y[1]);
	FbColor(BLACK);
	FbLine(q->qline[t].x[0], q->qline[t].y[0], q->qline[t].x[1], q->qline[t].y[1]);
	FbPushBuffer();
}

static void qix_advance_point(int *p, int limit, int *vel)
{
	*p += *vel;
	if (*p < 0) {
		*vel = -*vel;
		*p = -*p;
	}
	if (*p >= limit) {
		*vel = -*vel;
		*p = limit - (*p - limit);
	}
}

static void qix_set_color(struct qixline *line, int color_angle)
{
	short rs, gs, bs;
	int r, g, b;
	int rangle, gangle, bangle;
	uint16_t color;

	rangle = color_angle;
	gangle = color_angle + (128 / 3);
	if (gangle > 127)
		gangle -= 128;
	bangle = gangle + (128 / 3);
	if (bangle > 127)
		bangle -= 128;
	rs = sine(rangle);
	gs = sine(gangle);
	bs = sine(bangle);

	/* map color values from [-256-255] to [0-512] */
	r = (int) rs + 256;
	g = (int) gs + 256;
	b = (int) bs + 256;

	r = r >> 4; /* top 5 bits */
	g = g >> 3; /* top 6 bits */
	b = b >> 4; /* top 5 bits */

	color = (r << 11) | (g << 5) | b;
	line->color = color;
}

static void move_qix(struct qix *q)
{
	q->qline[q->tail] = q->qline[q->head];
	qix_advance_point(&q->qline[q->tail].x[0], LCD_XSIZE, &q->vx[0]);
	qix_advance_point(&q->qline[q->tail].y[0], LCD_YSIZE, &q->vy[0]);
	qix_advance_point(&q->qline[q->tail].x[1], LCD_XSIZE, &q->vx[1]);
	qix_advance_point(&q->qline[q->tail].y[1], LCD_YSIZE, &q->vy[1]);
	qix_set_color(&q->qline[q->tail], q->color_angle);
	q->head = q->tail;
	q->tail = (q->tail + 1) % QIX_LINE_COUNT;
	q->color_angle++;
	if (q->color_angle > 127)
		q->color_angle = 0;
}

void qix(void)
{
	if (!the_qix.initialized)
		init_qix(&the_qix);
	draw_qix(&the_qix);
	move_qix(&the_qix);
}

