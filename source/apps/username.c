

#include <string.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "badge.h"
#include "framebuffer.h"
#include "key_value_storage.h"

/* username is a global externally visible */
#define NAMESIZE 10
char username[NAMESIZE] = { 0 };

/* Restore username from flash.  uname is a pointer to the memory
 * in RAM to which the data should be stored. The user name is length
 * characters, not necessarily NULL terminated.
 */
void restore_username_from_flash(char *uname, int length) {
    if (length > (int)sizeof(badge_system_data()->name)) {
        length = sizeof(badge_system_data()->name);
    }
    memcpy(uname, badge_system_data()->name, length);
}

/* Restore username from flash.  uname is a pointer to the memory
 * in RAM to which the data should be stored. The user name is length
 * characters, not necessarily NULL terminated.
 */
void save_username_to_flash(char *uname, int length) {
   memcpy(badge_system_data()->name, uname, length);
   flash_kv_store_binary("sysdata", badge_system_data(), length);
}

#define INIT_APP_STATE 0
#define DRAW_SCREEN 1
#define RENDER_SCREEN 2
#define CHECK_THE_BUTTONS 3
#define EXIT_APP 4

static void app_init(void);
static void draw_screen(void);
static void render_screen(void);
static void check_the_buttons(void);
static void exit_app(void);

typedef void (*state_to_function_map_fn_type)(void);

static state_to_function_map_fn_type state_to_function_map[] = {
	app_init,
	draw_screen,
	render_screen,
	check_the_buttons,
	exit_app,
};

static int current_row = 0;
static int current_col = 0;
static int cursor = 0;
static int app_state = INIT_APP_STATE;
static int something_changed = 0;

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

static void draw_box(int row, int col)
{
	int x1, y1, x2, y2;

	x1 = 10 + col * 18 - 3;
	x2 = 10 + (col + 1) * 18 - 10;
	y1 = 30 + row * 20 - 2;
	y2 = 30 + (row + 1) * 20 - 10;

	if (row == 4 && col == 5) { /* Special case for EXIT */
		x2 = x2 + 18;
		x1 = x1 - 18;
	}

	FbHorizontalLine(x1, y1, x2, y1);
	FbHorizontalLine(x1, y2, x2, y2);
	FbVerticalLine(x1, y1, x1, y2);
	FbVerticalLine(x2, y1, x2, y2);
}

static void draw_screen(void)
{
	int i, row, col, x, y;
	char letter;
	char buf[2];

	FbClear();
	FbMove(10, 3);
	FbWriteLine("ENTER NAME:");
	/* Draw current name */
	for (i = 0; i < NAMESIZE; i++) {
		if (i == cursor)
			FbColor(GREEN);
		else
			FbColor(WHITE);
		buf[0] = '_';
		buf[1] = '\0';
		if (username[i] >= 'A' && username[i] <= 'Z')
			buf[0] = username[i];
		FbMove(10 + i * 10, 15);
		FbWriteLine(buf);
	}
	FbColor(WHITE);
	letter = 'A';
	for (row = 0; row < 5; row++) {
		for (col = 0; col < 6; col++) {
			x = 10 + col * 18;
			y = 30 + row * 20;
			FbMove(x, y);
			buf[0] = letter;
			buf[1] = '\0';
			FbWriteLine(buf);
			if (letter == '<') {
				if (current_col == col && current_row == row)
					draw_box(row, col);
				break;
			}
			if (letter == '_')
				letter = '<';
			else if (letter < 'Z')
				letter++;
			else
				letter = '_';
			if (current_col == col && current_row == row)
				draw_box(row, col);
		}
	}
	x = 10 + 4 * 18;
	y = 30 + 4 * 20;
	FbMove(x, y);
	FbWriteLine("EXIT");
	if (current_row == 4 && current_col >= 4) {
		current_col = 5;
		draw_box(current_row, current_col);
	}
	app_state = RENDER_SCREEN;
}

static void render_screen(void)
{
	if (something_changed)
		FbSwapBuffers();
	something_changed = 0;
	app_state = CHECK_THE_BUTTONS;
}

static int row_col_to_letter(int row, int col)
{
	int c = row * 6 + col;
	if (c >= 0 && c < 26)
		return c + 'A';
	if (c == 26)
		return '_';
	if (c == 27)
		return '<';
	return -1;
}

static void check_the_buttons(void)
{
	int action;
    int down_latches = button_down_latches();

	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		current_row--;
		if (current_row < 0)
			current_row = 4;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		current_row++;
		if (current_row > 4)
			current_row = 0;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		if (current_col == 5 && current_row == 4) /* special case for exit */
			current_col--;
		current_col--;
		if (current_col < 0)
			current_col = 5;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		current_col++;
		if (current_col > 5)
			current_col = 0;
		something_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		action = row_col_to_letter(current_row, current_col);
		if (action == -1) { /* exit */
			app_state = EXIT_APP;
			return;
		} else if (action == '<') { /* backspace */
			if (cursor > 0) {
				if (cursor == NAMESIZE - 1 && username[cursor] != '\0') {
					username[cursor] = '\0';
					something_changed = 1;
					return;
				}
				cursor--;
				username[cursor] = '\0';
				something_changed = 1;
			}
		} else {
			username[cursor] = action;
			cursor++;
			if (cursor > NAMESIZE - 1)
				cursor = NAMESIZE - 1;
			something_changed = 1;
		}
	}
	if (something_changed)
		app_state = DRAW_SCREEN;
        return;
}

static void exit_app(void)
{
	save_username_to_flash(username, sizeof(username));
	app_state = INIT_APP_STATE;
	returnToMenus();
}

static void app_init(void)
{
	restore_username_from_flash(username, sizeof(username));
	FbInit();
	FbClear();
	FbColor(WHITE);
	app_state = INIT_APP_STATE;
	something_changed = 1;
	app_state = DRAW_SCREEN;
}

int username_cb(void)
{
	state_to_function_map[app_state]();
	return 0;
}
