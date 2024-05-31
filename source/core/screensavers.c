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

#define SCREEN_ORIENTATION_LANDSCAPE 0 /* 0 = portrait, 1 = landscape */

#define DEFINE_IMAGE_ASSET_DATA
#include "holly.h"
#undef DEFINE_IMAGE_ASSET_DATA

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

void nametag_screensaver(void)
{
	int len;
	char name[11];

	draw_hyperspace_screensaver();
	FbColor(YELLOW);
	FbBackgroundColor(BLACK);
#if SCREEN_ORIENTATION_LANDSCAPE
# define WRITEF FbRotWriteString
	FbMove(LCD_XSIZE - 20, 10);
	WRITEF("HELLO MY NAME IS\n");
#else
# define WRITEF FbWriteString
	FbMove(10, 20);
	WRITEF("HELLO\nMY NAME IS\n");
#endif
	strncpy(name, badge_system_data()->name, 11);
	name[10] = '\0';
	len = strlen(name);

	/* Cut off trailing spaces */
	for (int i = len - 2; i >= 0; i--) {
		if (name[i] == ' ')
			name[i] = '\0';
		else
			break;
	}
	len = strlen(name);

#if SCREEN_ORIENTATION_LANDSCAPE
	int y = ((LCD_YSIZE / 2) - (len * 9) / 2);
	FbMove(64, y);
#else
	int x = ((LCD_XSIZE / 2) - (len * 9) / 2);
	FbMove(x, 80);
#endif
	WRITEF(name);
	FbSwapBuffers();
#undef WRITEF
}

void disp_asset_saver(void)
{
    extern const struct asset2 RVAsec_13;

    static unsigned char imgnum = 0;
    if(!animation_count){
        uint8_t random;
        random_insecure_bytes(&random, sizeof(int8_t));

        imgnum = random % 4;
        animation_count++;
    }

    switch(imgnum){
        case 1:
            FbMove(0,0);
            FbImage(&assetList[HACKRVA4], 0);
            break;

	case 0:
        case 2:
            FbMove(0,0);
            FbImage2(&RVAsec_13, 0);
            break;

        case 3:
            render_screen_save_monsters();
            break;
    }

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
        FbColor(animation_count < (16 * 16) ? BLUE << (animation_count / 16)
                                            : BLACK);
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
		if (y[i] * 8 < LCD_YSIZE - 8) /* not bottom row? */
			FbColor(x11_light_green);
		else
			FbColor(x11_dark_green);
		FbBackgroundColor(BLACK);
		unsigned char ch = random_num(63) + 'A';
		FbMove(x[i] * 8, y[i] * 8);
		FbCharacter(ch);
		if (random_num(1000) < 900) {
			int ny = y[i] - 1;
			if (ny >= 0) { /* if it's not off the top of the screen */
				FbColor(x11_dark_green);
				FbMove(x[i] * 8, ny * 8);
				ch = random_num(63) + 'A';
				FbCharacter(ch);
			}
		}
		y[i] += 1;
		if (y[i] > (LCD_YSIZE - 8) / 8) { /* hit bottom of screen? */
			/* choose new location, random x, y = top of screen */
			x[i] = random_num(LCD_XSIZE / 8);
			y[i] = 0;
		}
	}
	FbPushBuffer();
}

const char badgetips_header[] = "--Badge Tip--";
//const unsigned char badgetip_more_you_know = 
void just_the_badge_tips(void)
{
    static unsigned char tipnum = 0;
    if(!animation_count){
        uint8_t random;
        random_insecure_bytes(&random, sizeof(random));
        tipnum = (tipnum + random) % 19;
        animation_count++;
    }

    FbBackgroundColor(BLACK);
    FbColor(GREEN);
#if SCREEN_ORIENTATION_LANDSCAPE
    /* landscape orientation */
#   define WRITE_TIP FbRotWriteString
    FbMove(LCD_XSIZE - 8 - 4, 14);
#else
#   define WRITE_TIP FbWriteString
    FbMove(14, 4);
#endif
    WRITE_TIP(badgetips_header);

    FbColor(YELLOW);
#if SCREEN_ORIENTATION_LANDSCAPE
    FbMove(LCD_XSIZE - 20 - 20 , 4);
#else
    FbMove(4, 20);
#endif
    switch(tipnum){
        case 0:
            WRITE_TIP("Do NOT lick\nthe badge.");
            break;
        case 1:
            WRITE_TIP("These are land\ndwelling badges.\nAvoid water.");
            break;
        case 2:
            WRITE_TIP("Known to the\nstate of cancer\nto cause\nCalifornia.");
            break;
        case 3:
            WRITE_TIP("Wash your hands\nevery time.");
            break;
        case 4:
            WRITE_TIP("Say \"hi\" to the\ncreators over\nat HackRVA!");
            break;
        case 5:
            WRITE_TIP("I gotta pee.");
            break;
        case 6:
            WRITE_TIP("This badge is\nAI powered.");
            break;
        case 7:
            WRITE_TIP("This badge will\nself-destruct\nin 10 seconds.");
            break;
        case 8:
            WRITE_TIP("Checkout CTF!\n\n8CC36AE8\n862F8B73\nC32DCF62\n745B3303");
            break;
        case 9:
            WRITE_TIP("Its impolite\nto stare at\nother peoples\nbadges.");
            break;
        case 10:
            WRITE_TIP("Every badge is\ndifferent. Find\nthe bugs in\nyours!");
            break;
        case 11:
            WRITE_TIP("Badges are\nvery social.\nTry playing\nIR games.");
            break;
        case 12:
            WRITE_TIP("Its a thin line\nbetween a badge\nand an bodge.");
            break;
        case 13:
            WRITE_TIP("If you can't\nread this then\nyour badge is\nbroken.");
            break;
        case 14:
            WRITE_TIP("Youll find the\nsource code\nonline on\nGitHub.");
            break;
        case 15:
            WRITE_TIP("The badge is\nnot a touch\nscreen.");
            break;
        case 16:
            WRITE_TIP("Badges need\nlove. Pet\nyour badge\nregularly.");
            break;
        case 17:
            WRITE_TIP("These are\nartisanal\nbadges.");
            break;
        case 18:
        default:
            WRITE_TIP("Badges are\nhand crafted\nat HackRVA.");
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

#undef SCREEN_ORIENTATION_LANDSCAPE
