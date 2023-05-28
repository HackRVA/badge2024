#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "random.h"
#include "xorshift.h"

/* Program states.  Initial state is PONG_INIT */
enum pong_state_t {
	PONG_INIT,
	PONG_RUN,
	PONG_EXIT,
};

static enum pong_state_t pong_state = PONG_INIT;

#define PADDLE_WIDTH 16
#define PADDLE_SPEED (256 * 7)
static struct paddle {
	int x, y, vx;
} paddle[2] = { 0 };

#define BALL_SPEED (150)
static struct ball {
	int x, y, vx, vy;
} ball = { -1, -1, 0, 0 };
#define SERVE_TIME 60
static int serve_timer = 1000;
static int serving_player = 0;

static int score[2] = { 0 };

static int random_num(int n)
{
	int x;
	static unsigned int state = 0;

	assert(n != 0);
	if (state == 0)
		random_insecure_bytes((uint8_t *) &state, sizeof(state));
	x = xorshift(&state);
	if (x < 0)
		x = -x;
	return x % n;
}

static void pong_init(void)
{
	FbInit();
	FbClear();
	pong_state = PONG_RUN;
	memset(score, 0, sizeof(score));
	paddle[0].x = 256 * (LCD_XSIZE / 2);
	paddle[0].y = 256 * 3;
	paddle[0].vx = 0;
	paddle[1].x = 256 * (LCD_XSIZE / 2);
	paddle[1].y = 256 * (LCD_YSIZE - 4);
	paddle[1].vx = 0;
	serve_timer = SERVE_TIME;
}

static void move_paddle(struct paddle *p)
{
	p->x += p->vx;
	if (p->x < 256 * PADDLE_WIDTH / 2)
		p->x = 256 * PADDLE_WIDTH / 2;
	else if (p->x > 256 * (LCD_XSIZE - PADDLE_WIDTH / 2))
		p->x = 256 * (LCD_XSIZE - PADDLE_WIDTH / 2);
	if (p->vx > 0)
		p->vx -= PADDLE_SPEED;
	if (p->vx < 0)
		p->vx += PADDLE_SPEED;
}

static void move_paddles(void)
{
	move_paddle(&paddle[0]);
	move_paddle(&paddle[1]);
}

static void draw_paddle(struct paddle *p)
{
	int x1, x2, y;

	x1 = p->x / 256 - PADDLE_WIDTH / 2;
	x2 = p->x / 256 + PADDLE_WIDTH / 2;
	y = p->y / 256;
	FbColor(WHITE);
	FbHorizontalLine(x1, y, x2, y);
}

static void draw_net(void)
{
	for (int i = 0; i < LCD_XSIZE / 10; i++) {
		FbHorizontalLine(i * (LCD_XSIZE / 10), LCD_YSIZE / 2,
				i * (LCD_XSIZE / 10) + LCD_XSIZE / 20, LCD_YSIZE / 2);
	}
}

static void draw_paddles(void)
{
	draw_paddle(&paddle[0]);
	draw_paddle(&paddle[1]);
}

static void draw_ball(void)
{
	int x, y;

	x = ball.x / 256;
	y = ball.y / 256;

	FbColor(WHITE);
	FbPoint(x, y);
}

static void check_buttons(void)
{
	int down_latches = button_down_latches();

	int r0 = button_get_rotation(0);
	int r1 = button_get_rotation(1);

	if (r0)
		paddle[0].vx += r0 * PADDLE_SPEED;
	if (r1)
		paddle[1].vx -= r1 * PADDLE_SPEED;

	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		/* Pressing the button exits the program. You probably want to change this. */
		pong_state = PONG_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	}
}

static void serve_ball(int serving_player)
{
	ball.y = 256 * (LCD_YSIZE / 2);
	ball.x = 256 * (LCD_XSIZE / 2);
	if (serving_player == 0) {
		ball.vy = 3 * BALL_SPEED;
		ball.vx = random_num(4 * BALL_SPEED) - 2 * BALL_SPEED;
	} else {
		ball.vy = -3 * BALL_SPEED;
		ball.vx = random_num(4 * BALL_SPEED) - 2 * BALL_SPEED;
	}
}

static void move_ball(void)
{
	int oy, x, y, p1x, p1y, p2x, p2y;

	oy = ball.y / 256;

	ball.x += ball.vx;
	ball.y += ball.vy;

	x = ball.x / 256;
	y = ball.y / 256;
	if (y < 0) {
		score[0]++;
		serve_timer = SERVE_TIME;
		serving_player = 1;
		return;
	}
	if (y >= LCD_YSIZE) {
		score[1]++;
		serve_timer = SERVE_TIME;
		serving_player = 0;
		return;
	}
	if (x < 0) {
		x = 0;
		ball.vx = -ball.vx;
	}
	if (x >= LCD_XSIZE) {
		x = LCD_XSIZE - 1;
		ball.vx = -ball.vx;
	}

	p1x = paddle[0].x / 256;
	p1y = paddle[0].y / 256;
	p2x = paddle[1].x / 256;
	p2y = paddle[1].y / 256;

	if (oy >= p1y && y <= p1y && x >= p1x - PADDLE_WIDTH / 2 &&
			x <= p1x + PADDLE_WIDTH / 2 && ball.vy < 0) {
		ball.vy = -ball.vy;
		/* Impart some paddle energy to the ball */
		ball.vx += paddle[0].vx / 2;
	}
	if (oy <= p2y && y >= p2y && x >= p2x - PADDLE_WIDTH / 2 &&
		x <= p2x + PADDLE_WIDTH / 2 && ball.vy > 0) {
		ball.vy = -ball.vy;
		/* Impart some paddle energy to the ball */
		ball.vx += paddle[0].vx / 2;
	}
}

static void draw_screen(void)
{
	char s[10];

	FbColor(WHITE);
	if (serve_timer > 0) {
		serve_timer--;
		if (serve_timer == 1)
			serve_ball(serving_player);
	} else {
		move_ball();
	}
	move_paddles();
	draw_paddles();
	draw_net();
	if (serve_timer == 0)
		draw_ball();
	snprintf(s, sizeof(s), "%d", score[0]);
	FbMove(3, 3);
	FbWriteString(s);
	snprintf(s, sizeof(s), "%d", score[1]);
	FbMove(3, LCD_YSIZE - 10);
	FbWriteString(s);
	FbSwapBuffers();
}

static void pong_run(void)
{
	check_buttons();
	draw_screen();
}

static void pong_exit(void)
{
	pong_state = PONG_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

void pong_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (pong_state) {
	case PONG_INIT:
		pong_init();
		break;
	case PONG_RUN:
		pong_run();
		break;
	case PONG_EXIT:
		pong_exit();
		break;
	default:
		break;
	}
}

