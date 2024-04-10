#include <stdio.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"

/* Return world index (0 - 4095) from (x, y) coords */
static inline int windex(int x, int y)
{
	return (y * 64) + x;
}

struct badgey_world {
	char *name;
	char const *wm; /* world map */
	struct badgey_world *subworld[10];
};

static const char space_place[4096] = {
	"                                                                " /* 0 */
	"                                                      *         "
	"      *        1                                                "
	"                                                                "
	"                             *                        0         "
	"                                                                "
	"                 *                           *                * "
	"                                                                "
	"  *                                                             "
	"                                             *                  "
	"                                                                " /* 10 */
	"              *                         *                       "
	"                                                    2           "
	"                                                                "
	"                                                                "
	"                                                                "
	"        *           *    *                                      "
	"                                                                "
	"                                         *               *      "
	"                                                                "
	"                                                                " /* 20 */
	"                                                                "
	"                                                                "
	"                  *                                3            "
	"                                                                "
	"                                                                "
	"                                                                "
	"                                *                               "
	"                                                                "
	"                                                                "
	"                                                                " /* 30 */
	"        *                                     *                 "
	"                                     *                          "
	"    5                                                           "
	"                                                          *    *"
	"                  *                                             "
	"                                *                               "
	"*                                                               "
	"                                                                "
	"                                                                "
	"         *                                         *            " /* 40 */
	"                                                                "
	"                                                                "
	"                                                                "
	"                                   *                            "
	"                *                                               "
	"                                                                "
	"                                                                "
	"                                                                "
	"         *                    *                                 "
	"                                                                " /* 50 */
	"                                                                "
	"                                             *                  "
	"   *                                                            "
	"                                                                "
	"                           *                                    "
	"                                                                "
	"                                 4                              "
	"*               *                                  *            "
	"                                                                "
	"                                                   *            " /* 60 */
	"                        *                 *                     "
	"       *                                                        "
	"                                                                ", /* 63 */
};

static const char ossaria_place[4096] = {
	"....wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	".........wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"ww.........wwwwwwwwwwwwwwwwwwwwwwww..wwww..wwwwwwwwwww.wwwwwwwww"
	"ww..........ffmmmmmf.....wwwwwwwwwwwwww....wwwwwwwwwww....wwwwww"
	"wwwww.......ffm..fmff.......wwwwwwwwwwwwwwwwwwwwwwww..........ww"
	"wwwwww....ffffmmffmmf.......wwwwwwwwwwwwwwwwwww..........wwwwwww"
	"wwwwww.....ff..mmff.ff.....wwww...................wwwwwwwwwwwwww"
	"wwwwww......f..fmm..ff.....w...................wwwwwwwwwwwwwwwww"
	"wwwww..........ff.................................wwwwwwwwwwwwww"
	"wwwww...........f.............................wwwwwwwwwwwwwwwwww"
	"wwwwww..w................................wwwwwwwwwwwwwwwwwwwwwww"
	"wwwww...ww...................wwwwwwm..................wwwwwwwwww"
	"wwwwwwwww.................f.wwwwwmmm................wwwwww.wwwww"
	"wwwww..ww................fffwwwmmm................wwwwwww..wwwww"
	"wwwwww..www.............ffffffmm.....................w.....wwwww"
	"wwwww.....................fffffmmfff....................wwwwwwww"
	"wwwww......................fffffmffffff................wwwwwwwww"
	"www....fmf...................ffffffff..................wwwwwwwww"
	"www.....mmf.............fff....fffff......................wwwwww"
	"wwwww...fmmf....................fffff.................www.wwwwww"
	"wwwww...ffmfff....................fffff...............wwwwwwwwww"
	"wwwww...f...ffff........................................wwwwwwww"
	"wwwww.w......f.ff.......................................wwwwwwww"
	"wwwwwww........fff....................................w.wwww..ww"
	"wwwwwwww.......ffff...................................wwwww...ww"
	"wwwwww...........f..f.................................wwww....ww"
	"wwwww..............ff......................f...........ww.....ww"
	"wwwwww...........................mm..m....ff................wwww"
	"wwwww...............f........m..mm...mm...ff................wwww"
	"wwwww........................mmmm.....mmmff.............wwwwwwww"
	"wwwwww......................mmm........mmfffff........wwwwwwwwww"
	"wwwwww......................m...........mf..ff........wwwwwwwwww"
	"wwwwwww.....................m..........mmf...f..........wwwwwwww"
	"wwwwwwwwww..................m................ff.......wwwwwwwwww"
	"wwwwwwwww...........................mmmmmm....f.......wwwwwwwwww"
	"wwww..w.......................mmmmmmm...m...f.f............wwwww"
	"wwww.........................mmm.......ff..ff..............wwwww"
	"wwwwww......................ffffff...fff...ff...........w.wwwwww"
	"wwwwww....................fffff.....fff....f....ww....wwwwwwwwww"
	"wwwww....................fffffff....fff..........www.....wwwwwww"
	"wwwww..................fffffff.....................wwwwwwwwwwwww"
	"wwwww...................fffffff....................ww..wwwwwwwww"
	"wwwww...............fffffffffffffffffffff........www.....wwwwwww"
	"wwwwww.................fffffffffffffffffffff...........wwwwwwwww"
	"wwwwww........................fffffffffffffffffffff...wwwwwwwwww"
	"wwwwww.......................ffffffffffff..............wwwwwwwww"
	"wwwwwww.................fffffff.....fffffff................wwwww"
	"wwwwwwww...........fffffff................fffffff........wwwwwww"
	"wwwwwwwww..www..........................................wwwwwwww"
	"wwwwwwwwww.w..........................................wwwwwwwwww"
	"wwwwww..wwwwwww.............................................wwww"
	"wwwww......w................mmm........................w.....www"
	"wwwwwww....w.............wwwwwmm..................w....www...www"
	"wwwwwwww...ww.......ffff....wwwwww................w.w..wwwwwwwww"
	"wwwwwwww..........ffff........wwwww.....m.......w.www...wwwwwwww"
	"wwwww...........ffffffff........www.....mm......www......wwwwwww"
	"wwwww.........fffffffffffffff..wwww......mmm......w......wwwwwww"
	"wwwwww....w..fffff..........wwwwwwwwww.....m.....wwwwwwwwwwwwwww"
	"wwwwwww..ww..........wwww...wwwwwwwwwwwwwwwww............wwwwwww"
	"wwwwwww.wwww.....wwwwwwwwww.wwwwwwwwwwwwwwwwwwwww.........wwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww.........www"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww.....wwwwwwwwwwwwww......www"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww.....w"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww......"
};

static const struct badgey_world space = {
	"SPACE",
	space_place,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

static const struct badgey_world ossaria = {
	"OSSARIA",
	ossaria_place,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

static struct player {
	struct badgey_world const *world; /* current world */
	int x, y; /* coords in current world */
} player = {
	.world = &ossaria,
	.x = 32,
	.y = 32,
};

/* Program states.  Initial state is BADGEY_INIT */
enum badgey_state_t {
	BADGEY_INIT,
	BADGEY_RUN,
	BADGEY_EXIT,
};

static enum badgey_state_t badgey_state = BADGEY_INIT;
static int screen_changed = 0;

static void badgey_init(void)
{
	FbInit();
	FbClear();
	badgey_state = BADGEY_RUN;
	screen_changed = 1;
}

static void check_buttons(void)
{
	int newx, newy;

	newx = player.x;
	newy = player.y;

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		newx--;
		if (newx < 0)
			newx += 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		newx++;
		if (newx > 63)
			newx -= 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		newy--;
		if (newy < 0)
			newy += 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		newy++;
		if (newy > 63)
			newy -= 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		badgey_state = BADGEY_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		badgey_state = BADGEY_EXIT;
	}
	if (player.world == &space) {
		/* Prevent player from driving into a star */
		if (player.world->wm[windex(newx, newy)] == '*') {
			return;
		}
	} else {
		/* Prevent player from traversing water or mountains */
		if (player.world->wm[windex(newx, newy)] == 'w' ||
			player.world->wm[windex(newx, newy)] == 'm') {
			return;
		}
	}
	if (newx != player.x || newy != player.y) {
		player.x = newx;
		player.y = newy;
		screen_changed = 1;
	}
}

/* Draw a cell of the world at screen coords (x, y) */
static void draw_cell(int x, int y, unsigned char c)
{
	FbMove(x, y);
	switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (player.world == &space) { /* player is in space?  it's a planet */
			FbCharacter((unsigned char) 'O');
		} else {
			/* Not in space, so ... */
			if ((c - '0') % 2 == 0)
				FbCharacter((unsigned char) 'T'); /* ... it's a town ... */
			else
				FbCharacter((unsigned char) 'C'); /* ... or it's a cave. */
		}
		break;
	case '*':
		FbCharacter(c);
		break;
	case ' ': /* empty space, no need to draw anything */
		break;
	case '.':
		FbColor(GREEN);
		FbCharacter(c);
		break;
	case 'w':
		FbColor(BLUE);
		FbCharacter(c);
		break;
	case 'f':
		FbColor(x11_olive_drab);
		FbCharacter(c);
		break;
	case 'm':
		FbColor(x11_gray);
		FbCharacter(c);
		break;
	case 'B':
		FbColor(WHITE);
		FbCharacter(c);
		break;
	default:
		FbCharacter((unsigned char) '?'); /* unknown */
		break;
	}
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbClear();
	FbColor(WHITE);

	int x, y, sx, sy;

	x = player.x - 3;
	if (x < 0)
		x += 64;
	y = player.y - 4;
	if (y < 0)
		y += 64;
	sx = 0;
	sy = 0;
	int count = 0;
	do {
		unsigned char c = (unsigned char) player.world->wm[windex(x, y)];
		draw_cell(sx, sy, c);
		x++;
		count++;
		if (x > 63)
			x -= 64;
		sx += 16;
		if ((count % 7) == 0) {
			if (count == 7 * 9)
				break;
			x = player.x - 3;
			if (x < 0)
				x += 64;
			sx = 0;
			y++;
			sy += 16;
			if (y > 63) {
				y = y - 64;
			}
		}
	} while (1);

	draw_cell(16 * 3, 16 * 4, 'B');

	char buf[20];
	snprintf(buf, sizeof(buf), "(%d, %d)", player.x, player.y);
	FbMove(0, 152);
	FbWriteString(buf);

	screen_changed = 0;
	FbPushBuffer();
}

static void badgey_run(void)
{
	check_buttons();
	draw_screen();
}

static void badgey_exit(void)
{
	badgey_state = BADGEY_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

/* You will need to rename badgey_cb() something else. */
void badgey_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (badgey_state) {
	case BADGEY_INIT:
		badgey_init();
		break;
	case BADGEY_RUN:
		badgey_run();
		break;
	case BADGEY_EXIT:
		badgey_exit();
		break;
	default:
		break;
	}
}

