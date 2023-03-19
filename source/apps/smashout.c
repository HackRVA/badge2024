/*********************************************

 Smashout game (like breakout) for HackRVA 2020 badge

 Author: Stephen M. Cameron <stephenmcameron@gmail.com>
 (c) 2019 Stephen M. Cameron

**********************************************/
#include <stdio.h>
#include <string.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "rtc.h"
#include "xorshift.h"

/* TODO figure out where these should really come from */
#define PADDLE_HEIGHT 12
#define PADDLE_WIDTH 30
#define PADDLE_SPEED 7
#define BALL_STARTX (8 * LCD_XSIZE / 2)
#define BALL_STARTY (8 * LCD_YSIZE / 3)
#define BALL_START_VX (5 * ((xorshift(&xorshift_state) % 11) - 5))
#define BALL_START_VY (2 * 8)
#define BRICK_WIDTH 8
#define BRICK_HEIGHT 4
#define SPACE_ABOVE_BRICKS 20
#define BALL_RADIUS 2

static struct smashout_paddle {
	int x, y, vx;
	char w;
} paddle, old_paddle;

static struct smashout_ball {
	int x, y, vx, vy;
} ball, oldball;

static struct smashout_brick {
	unsigned char x, y, alive; /* upper left corner of brick */
} brick[4 * 16];

/* When a brick is destroyed, it emits sparks.  On linux, it looks good
 * if the sparks are the same color as the destroyed brick.  On the badge
 * you really can't see the sparks unless they're white
 */
#define USE_COLORED_SPARKS 0
#define SPARKS_PER_BRICK 8
#define MAXSPARKS 25
static struct spark_data {
	int x, y, vx, vy, alive;
#if USE_COLORED_SPARKS
#define SPARK_COLOR spark[i].color
	int color;
#else
#define SPARK_COLOR WHITE
#endif
} spark[MAXSPARKS] = { 0 };

static int score = 0;
static int balls = 0;
static int score_inc = 0;
static int balls_inc = 0;
static unsigned int xorshift_state;

/* Program states.  Initial state is SMASHOUT_GAME_INIT */
enum smashout_program_state_t {
	SMASHOUT_GAME_INIT,
	SMASHOUT_GAME_PLAY,
	SMASHOUT_GAME_EXIT,
};

static int brick_color[4] = { YELLOW, GREEN, RED, BLUE };

static enum smashout_program_state_t smashout_program_state = SMASHOUT_GAME_INIT;

static void init_bricks(void)
{
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 16; j++) {
			brick[i * 16 + j].x = j * BRICK_WIDTH;
			brick[i * 16 + j].y = SPACE_ABOVE_BRICKS + i * BRICK_HEIGHT;
			brick[i * 16 + j].alive = 1;
		}
	}
}

static void smashout_game_init(void)
{
	xorshift_state = 0xa5a5a5a5 ^ (int)rtc_get_ms_since_boot();
	FbInit();
	FbClear();
	init_bricks();
	paddle.x = LCD_XSIZE / 2;
	paddle.y = LCD_YSIZE - PADDLE_HEIGHT;
	paddle.w = PADDLE_WIDTH;
	paddle.vx = 0;
	old_paddle = paddle;
	ball.x = BALL_STARTX;
	ball.y = BALL_STARTY;
	ball.vx = BALL_START_VX;
	ball.vy = BALL_START_VY;
	oldball = ball;
	smashout_program_state = SMASHOUT_GAME_PLAY;
}

static void smashout_check_buttons()
{
	int rotary_switch = button_get_rotation(0);
	int down_latches = button_down_latches();

	if (BUTTON_PRESSED(BADGE_BUTTON_SW, down_latches))
		smashout_program_state = SMASHOUT_GAME_EXIT;
	else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches))
		paddle.vx = -PADDLE_SPEED;
	else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
		paddle.vx = PADDLE_SPEED;
	if (rotary_switch)
		paddle.vx += PADDLE_SPEED * rotary_switch;
}

static void smashout_draw_paddle()
{
	int dx = old_paddle.w / 2;
	FbColor(BLACK);
	FbHorizontalLine(old_paddle.x - dx, old_paddle.y, old_paddle.x + dx, old_paddle.y);
	FbColor(WHITE);
	FbHorizontalLine(paddle.x - dx, paddle.y, paddle.x + dx, paddle.y);
}

static void smashout_draw_brick(int row, int col)
{
	struct smashout_brick *b = &brick[row * 16 + col];
	if (b->alive)
		FbColor(brick_color[row]);
	else
		FbColor(BLACK);
	FbMove(b->x, b->y);
	FbRectangle(BRICK_WIDTH, BRICK_HEIGHT);
}

static void smashout_draw_bricks()
{
	int i, j;
	int count = 0;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 16; j++) {
			smashout_draw_brick(i, j);
			if (brick[i * 16 + j].alive)
				count++;
		}

	if (count == 0) /* Start over when all the bricks are dead. */
		smashout_program_state = SMASHOUT_GAME_INIT;
}

static int xlim(int x)
{
	if (x < 0)
		return 0;
	if (x >= LCD_XSIZE)
		return LCD_XSIZE - 1;
	return x;
}

static int ylim(int y)
{
	if (y < 0)
		return 0;
	if (y >= LCD_YSIZE)
		return LCD_YSIZE - 1;
	return y;
}

#if USE_COLORED_SPARKS
static void add_spark(int x, int y, int color)
#else
static void add_spark(int x, int y, __attribute__((unused)) int color)
#endif
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive) {
			spark[i].x = x;
			spark[i].y = y;
			spark[i].vx = (((xorshift(&xorshift_state) >> 16) & 0x0ff) - 128);
			spark[i].vy = (((xorshift(&xorshift_state) >> 16) & 0x0ff) - 128);
			spark[i].alive = 4 + ((xorshift(&xorshift_state) >> 16) & 0x7);
#if USE_COLORED_SPARKS
			spark[i].color = color;
#endif
			return;
		}
	}
}

static void add_sparks(int x, int y, int n, int color)
{
	int i;

	for (i = 0; i < n; i++)
		add_spark(x, y, color);
}

static void smashout_move_sparks(void)
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive)
			continue;
		spark[i].x += spark[i].vx;
		spark[i].y += spark[i].vy;
		if (spark[i].alive > 0) {
			spark[i].alive--;
			if (!spark[i].alive) {
				/* Erase dead sparks */
				int x0, y0, x1, y1;
				x0 = ((spark[i].x - 2 * spark[i].vx) >> 3);
				y0 = ((spark[i].y - 2 * spark[i].vy) >> 3);
				x1 = ((spark[i].x - spark[i].vx) >> 3);
				y1 = ((spark[i].y - spark[i].vy) >> 3);
				if (x0 >= 0 && x0 <= 127 && y0 >= 0 && y0 <= 127 &&
					x1 >= 0 && x1 <= 127 && y1 >= 0 && y1 <= 127) {
					FbColor(BLACK);
					FbLine(x0, y0, x1, y1);
				}
			}
		}
	}
}

static void smashout_draw_sparks(void)
{
	int i, x0, y0, x1, y1, x2, y2;

	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive)
			continue;
		x0 = ((spark[i].x - 2 * spark[i].vx) >> 3);
		y0 = ((spark[i].y - 2 * spark[i].vy) >> 3);
		x1 = ((spark[i].x - spark[i].vx) >> 3);
		y1 = ((spark[i].y - spark[i].vy) >> 3);
		x2 = ((spark[i].x) >> 3);
		y2 = ((spark[i].y) >> 3);
		if (x0 >= 0 && x0 <= 127 && y0 >= 0 && y0 <= 127 &&
			x1 >= 0 && x1 <= 127 && y1 >= 0 && y1 <= 127) {
			FbColor(BLACK);
			FbLine(x0, y0, x1, y1);
		}
		if (x1 >= 0 && x1 <= 127 && y1 >= 0 && y1 <= 127 &&
			x2 >= 0 && x2 <= 127 && y2 >= 0 && y2 <= 127) {
			FbColor(SPARK_COLOR);
			FbLine(x1, y1, x2, y2);
		}
	}
}

static void smashout_draw_ball()
{
	int x, y, x1, y1, x2, y2;
	static const int r = BALL_RADIUS;

	x = oldball.x / 8;
	y = oldball.y / 8;

	x1 = xlim(x - r); x2 = xlim(x + r);
	y1 = ylim(y - r); y2 = ylim(y + r);
	FbColor(BLACK);
	FbHorizontalLine(x1, y1, x2, y1);
	FbHorizontalLine(x1, y2, x2, y2);
	FbVerticalLine(x1, y1, x1, y2);
	FbVerticalLine(x2, y1, x2, y2);

	x = ball.x / 8;
	y = ball.y / 8;
	x1 = xlim(x - r); x2 = xlim(x + r);
	y1 = ylim(y - r); y2 = ylim(y + r);
	FbColor(WHITE);
	FbHorizontalLine(x1, y1, x2, y1);
	FbHorizontalLine(x1, y2, x2, y2);
	FbVerticalLine(x1, y1, x1, y2);
	FbVerticalLine(x2, y1, x2, y2);
}

static void smashout_move_paddle()
{
	old_paddle = paddle;
	paddle.x += paddle.vx;
	if (paddle.vx > 0)
		paddle.vx -= 1;
	if (paddle.vx < 0)
		paddle.vx += 1;
	if (paddle.x < PADDLE_WIDTH / 2) {
		paddle.x = PADDLE_WIDTH / 2;
		paddle.vx = 0;
	}
	if (paddle.x > LCD_XSIZE - PADDLE_WIDTH / 2) {
		paddle.x = LCD_XSIZE - PADDLE_WIDTH / 2;
		paddle.vx = 0;
	}
}

static void smashout_move_ball()
{
	oldball = ball;
	ball.x = ball.x + ball.vx;
	ball.y = ball.y + ball.vy;
	if (ball.x > 8 * LCD_XSIZE - 2 * 8) {
		ball.x = 8 * LCD_XSIZE - 2 * 8;
		if (ball.vx > 0)
			ball.vx = -ball.vx;
	}
	if (ball.y > 8 * LCD_YSIZE - 2 * 8) {
		ball.x = BALL_STARTX;
		ball.y = BALL_STARTY;
		ball.vx = BALL_START_VX;
		ball.vy = BALL_START_VY;
		balls_inc++;
	}
	if (ball.x < 8) {
		ball.x = 8;
		if (ball.vx < 0)
			ball.vx = -ball.vx;
	}
	if (ball.y < 8) {
		ball.y = 8;
		if (ball.vy < 0)
			ball.vy = -ball.vy;
	}
	if (ball.y >= (paddle.y - 2) * 8 && ball.y <= (paddle.y * 8)) {
		if (ball.x > (paddle.x - PADDLE_WIDTH / 2 - 1) * 8 &&
			ball.x < (paddle.x + PADDLE_WIDTH / 2 + 1) * 8) {
			/* Ball has hit paddle, bounce. */
			ball.vy = - ball.vy;
			/* Impart some paddle energy to ball. */
			ball.vx += paddle.vx * 4;
			if (ball.vx > 80)
				ball.vx = 80;
			if (ball.vx < -80)
				ball.vx = -80;
		}
	}
	if (ball.y > 8 * SPACE_ABOVE_BRICKS && ball.y < 8 * (SPACE_ABOVE_BRICKS + 4 * BRICK_HEIGHT)) {
		/* Figure out which brick we are intersecting */
		int col = ball.x / (BRICK_WIDTH * 8);
		int row = (ball.y - 8 * SPACE_ABOVE_BRICKS) / (8 * BRICK_HEIGHT);
		if (col >= 0 && col <= 15 && row >= 0 && row <= 3) {
			struct smashout_brick *b = &brick[row * 16 + col];
			if (b->alive) {
				b->alive = 0;
				score_inc++;
				ball.vy = -ball.vy;
#if USE_COLORED_SPARKS
				add_sparks(ball.x, ball.y, SPARKS_PER_BRICK, brick_color[row]);
#else
				add_sparks(ball.x, ball.y, SPARKS_PER_BRICK, WHITE);
#endif
			}
		}
	}
}

static void draw_score_and_balls(int color)
{
	char s[20], b[20];
	FbColor(color);
	sprintf( s, "%d", score);
	sprintf( b, "%d", balls);
	FbMove(10, LCD_YSIZE - 10);
	FbWriteLine(s);
	FbMove(70, LCD_YSIZE - 10);
	FbWriteLine(b);
}

static void smashout_draw_screen()
{
	smashout_draw_paddle();
	smashout_draw_ball();
	smashout_draw_sparks();
	smashout_draw_bricks();
	draw_score_and_balls(BLACK);
	score += score_inc;
	balls += balls_inc;
	score_inc = 0;
	balls_inc = 0;
	draw_score_and_balls(WHITE);
	FbPaintNewRows();
}

static void smashout_game_play()
{
	smashout_check_buttons();
	smashout_move_paddle();
	smashout_move_ball();
	smashout_move_sparks();
	smashout_draw_screen();
}

static void smashout_game_exit()
{
	smashout_program_state = SMASHOUT_GAME_INIT;
	returnToMenus();
}

int smashout_cb(void)
{
	switch (smashout_program_state) {
	case SMASHOUT_GAME_INIT:
		smashout_game_init();
		break;
	case SMASHOUT_GAME_PLAY:
		smashout_game_play();
		break;
	case SMASHOUT_GAME_EXIT:
		smashout_game_exit();
		break;
	default:
		break;
	}
	return 0;
}

