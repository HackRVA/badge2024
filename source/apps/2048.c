#include <assert.h>
#include <stdio.h>

#include "button.h"
#include "colors.h"
#include "framebuffer.h"
#include "menu.h"
#include "palette.h"
#include "ui.h"
#include "xorshift.h"

static enum twenty_forty_state_t {
	TWENTY_FORTY_EIGHT_INIT = 0,
	TWENTY_FORTY_EIGHT_RUN,
	TWENTY_FORTY_EIGHT_SHOW_HELP,
	TWENTY_FORTY_EIGHT_MENU,
	TWENTY_FORTY_EIGHT_POLL_INPUT,
	TWENTY_FORTY_EIGHT_DRAW_SCREEN,
	TWENTY_FORTY_EIGHT_GAME_OVER,
	TWENTY_FORTY_EIGHT_EXIT,
} twenty_forty_eight_state = TWENTY_FORTY_EIGHT_INIT;

#define NUM_MENU_ITEMS 4
#define MENU_ITEM_SPACING 30
#define MENU_ITEM_WIDTH 100
#define MENU_ITEM_HEIGHT 20
#define MENU_X (LCD_XSIZE / 2 - MENU_ITEM_WIDTH / 2)
#define MENU_Y (LCD_YSIZE / 2 - MENU_ITEM_HEIGHT / 2)
#define GRID_SIZE 4
#define TILE_MASK 0xF

static bool first_launch = true;
static const char *menu_items[NUM_MENU_ITEMS] = {
	"play game",
	"reset",
	"how to play",
	"exit",
};

static struct palette default_palette = {
	.colors =
		{
			PACKRGB888(0, 0, 0),
			PACKRGB888(127, 36, 84),
			PACKRGB888(28, 43, 83),
			PACKRGB888(0, 135, 81),
			PACKRGB888(171, 82, 54),
			PACKRGB888(96, 88, 79),
			PACKRGB888(195, 195, 198),
			PACKRGB888(255, 241, 233),
			PACKRGB888(237, 27, 81),
			PACKRGB888(250, 162, 27),
			PACKRGB888(247, 236, 47),
			PACKRGB888(93, 187, 77),
			PACKRGB888(81, 166, 220),
			PACKRGB888(131, 118, 156),
			PACKRGB888(241, 118, 166),
			PACKRGB888(252, 204, 171),
		},
};

static uint64_t board = 0;
static uint64_t prev_board = 0;
static unsigned int random_num_state = 0;

static int current_menu_item = 0;
static bool current_menu_item_selected = false;

static int tile_size;
static int tile_spacing = 2;
static int grid_x;
static int grid_y;

static int screen_changed = 0;

static int tile_scale[GRID_SIZE][GRID_SIZE] = {{100}};
static bool moved_tiles[GRID_SIZE][GRID_SIZE] = {false};
static int animation_step = 0;
static const int animation_duration = 5;

static int get_tile(int row, int col)
{
	int shift = (row * GRID_SIZE + col) * 4;
	return (board >> shift) & TILE_MASK;
}

static void set_tile(int row, int col, int value)
{
	int shift = (row * GRID_SIZE + col) * 4;
	board &= ~((uint64_t)TILE_MASK << shift);
	board |= (uint64_t)value << shift;
}

static int find_empty_positions(int empty_positions[GRID_SIZE * GRID_SIZE])
{
	int empty_count = 0;
	for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
		if (((board >> (i * 4)) & TILE_MASK) == 0) {
			empty_positions[empty_count++] = i;
		}
	}
	return empty_count;
}

static int random_num(int n)
{
	int x;

	assert(n != 0);
	x = xorshift(&random_num_state);
	if (x < 0)
		x = -x;
	return x % n;
}

static void spawn_tile(void)
{
	int empty_positions[GRID_SIZE * GRID_SIZE];
	int empty_count = find_empty_positions(empty_positions);
	if (empty_count == 0)
		return;
	int pos = empty_positions[random_num(empty_count)];
	int value = (random_num(10) < 9) ? 1 : 2;
	board |= (uint64_t)value << (pos * 4);
}

static void reset_moved_tiles(void)
{
	for (int row = 0; row < GRID_SIZE; row++) {
		for (int col = 0; col < GRID_SIZE; col++) {
			moved_tiles[row][col] = false;
		}
	}
}

static void reset_game(void)
{
	board = 0;
	spawn_tile();
	spawn_tile();
	reset_moved_tiles();
}

static bool merged_tiles[GRID_SIZE][GRID_SIZE] = {false};

static void slide_and_merge(int *tiles)
{
	int newTiles[GRID_SIZE] = {0}, index = 0;
	bool merged[GRID_SIZE] = {false};

	for (int i = 0; i < GRID_SIZE; i++) {
		if (tiles[i] != 0)
			newTiles[index++] = tiles[i];
	}

	for (int i = 0; i < GRID_SIZE - 1; i++) {
		if (newTiles[i] != 0 && newTiles[i] == newTiles[i + 1] &&
			!merged[i] && !merged[i + 1]) {
			newTiles[i]++;
			newTiles[i + 1] = 0;
			merged[i] = true;
		}
	}

	index = 0;
	for (int i = 0; i < GRID_SIZE; i++) {
		if (newTiles[i] != 0) {
			tiles[index] = newTiles[i];

			merged_tiles[index][i] = merged[i];
			index++;
		}
	}

	while (index < GRID_SIZE)
		tiles[index++] = 0;
}

static void move_left(void)
{
	for (int row = 0; row < GRID_SIZE; row++) {
		int tiles[GRID_SIZE];
		for (int col = 0; col < GRID_SIZE; col++)
			tiles[col] = get_tile(row, col);
		slide_and_merge(tiles);
		for (int col = 0; col < GRID_SIZE; col++)
			set_tile(row, col, tiles[col]);
	}
}

static void move_right(void)
{
	for (int row = 0; row < GRID_SIZE; row++) {
		int tiles[GRID_SIZE];
		for (int col = 0; col < GRID_SIZE; col++)
			tiles[GRID_SIZE - 1 - col] = get_tile(row, col);
		slide_and_merge(tiles);
		for (int col = 0; col < GRID_SIZE; col++)
			set_tile(row, col, tiles[GRID_SIZE - 1 - col]);
	}
}

static void move_up(void)
{
	for (int col = 0; col < GRID_SIZE; col++) {
		int tiles[GRID_SIZE];
		for (int row = 0; row < GRID_SIZE; row++)
			tiles[row] = get_tile(row, col);
		slide_and_merge(tiles);
		for (int row = 0; row < GRID_SIZE; row++)
			set_tile(row, col, tiles[row]);
	}
}

static void move_down(void)
{
	for (int col = 0; col < GRID_SIZE; col++) {
		int tiles[GRID_SIZE];
		for (int row = 0; row < GRID_SIZE; row++)
			tiles[GRID_SIZE - 1 - row] = get_tile(row, col);
		slide_and_merge(tiles);
		for (int row = 0; row < GRID_SIZE; row++)
			set_tile(row, col, tiles[GRID_SIZE - 1 - row]);
	}
}

static bool is_game_over(void)
{
	for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
		if (((board >> (i * 4)) & TILE_MASK) == 0)
			return false;
		int row = i / GRID_SIZE, col = i % GRID_SIZE;
		if (col < GRID_SIZE - 1 &&
			get_tile(row, col) == get_tile(row, col + 1))
			return false;
		if (row < GRID_SIZE - 1 &&
			get_tile(row, col) == get_tile(row + 1, col))
			return false;
	}
	return true;
}

static void previous_menu_item(void)
{
	current_menu_item--;
	if (current_menu_item < 0)
		current_menu_item = NUM_MENU_ITEMS - 1;
}

static void next_menu_item(void)
{
	current_menu_item++;
	if (current_menu_item >= NUM_MENU_ITEMS)
		current_menu_item = 0;
}

static void handle_menu_options(void)
{
	switch (current_menu_item) {
	case 0:
		twenty_forty_eight_state = TWENTY_FORTY_EIGHT_RUN;
		if (first_launch)
			reset_game();
		break;
	case 1:
		reset_game();
		break;
	case 2:
		twenty_forty_eight_state = TWENTY_FORTY_EIGHT_SHOW_HELP;
		break;
	case 3:
		twenty_forty_eight_state = TWENTY_FORTY_EIGHT_EXIT;
		break;
	default:
		break;
	}
}

static void twenty_forty_eight_init(void)
{
	FbInit();
	FbClear();

	twenty_forty_eight_state = TWENTY_FORTY_EIGHT_MENU;
	screen_changed = 1;

	tile_size =
		(((LCD_XSIZE - (GRID_SIZE + 1) * tile_spacing) / GRID_SIZE) /
			2) +
		16;
	grid_x = tile_spacing;
	grid_y = (LCD_YSIZE -
			 (tile_size * GRID_SIZE +
				 tile_spacing * (GRID_SIZE - 1))) /
		2;
}

static void move_tiles(void (*move_func)(void))
{
	reset_moved_tiles();
	prev_board = board;
	move_func();

	if (board != prev_board) {
		spawn_tile();
		for (int row = 0; row < GRID_SIZE; row++) {
			for (int col = 0; col < GRID_SIZE; col++) {
				if ((int)get_tile(row, col) !=
					(int)((prev_board >>
						      ((row * GRID_SIZE + col) *
							      4)) &
						TILE_MASK)) {
					moved_tiles[row][col] = true;
				}
			}
		}
	}
}

static void check_buttons(void)
{
	int down_latches = button_down_latches();

	if (twenty_forty_eight_state == TWENTY_FORTY_EIGHT_MENU) {
		current_menu_item_selected = false;
		if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
			previous_menu_item();
		} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
			next_menu_item();
		} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
			current_menu_item_selected = true;
			handle_menu_options();
		} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
			twenty_forty_eight_state = TWENTY_FORTY_EIGHT_EXIT;
		}
		return;
	}

	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		move_tiles(move_left);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		move_tiles(move_right);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		move_tiles(move_up);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		move_tiles(move_down);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		twenty_forty_eight_state = TWENTY_FORTY_EIGHT_MENU;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		twenty_forty_eight_state = TWENTY_FORTY_EIGHT_EXIT;
	}
	prev_board = board;
}

static void twenty_forty_eight_exit(void)
{
	twenty_forty_eight_state =
		TWENTY_FORTY_EIGHT_INIT; /* So that when we start again, we do
					    not immediately exit */
	returnToMenus();
}

static void draw_menu(void)
{
	for (int i = 0; i < NUM_MENU_ITEMS; i++) {
		int y_offset = (i - current_menu_item) * MENU_ITEM_SPACING;
		struct ui_button button = {
			.x = MENU_X,
			.y = MENU_Y + y_offset,
			.width = MENU_ITEM_WIDTH,
			.height = MENU_ITEM_HEIGHT,
			.text = menu_items[i],
			.outline_size = 3,
			.outline_color =
				palette_color_from_index(default_palette, 12),
			.fill_color =
				palette_color_from_index(default_palette, 2),
			.text_color =
				palette_color_from_index(default_palette, 7),
		};

		if (i == current_menu_item) {
			button.outline_color =
				palette_color_from_index(default_palette, 6);
			if (current_menu_item_selected)
				button.fill_color = palette_color_from_index(
					default_palette, 5);
		}

		ui_button_dither_fill(button, button.fill_color,
			palette_color_from_index(default_palette, 0), 1);
		ui_button_draw_outline(button, button.outline_color);
		ui_button_draw_label(button, button.text_color);
	}
}

static void update_tile_animation(void)
{
	if (animation_step < animation_duration) {
		for (int row = 0; row < GRID_SIZE; row++) {
			for (int col = 0; col < GRID_SIZE; col++) {
				if (moved_tiles[row][col]) {
					if (animation_step <
						animation_duration / 2) {
						tile_scale[row][col] = 80;
					} else {
						tile_scale[row][col] += 20 /
							(animation_duration /
								2);
					}
				}
			}
		}
		animation_step++;
	} else {
		for (int row = 0; row < GRID_SIZE; row++) {
			for (int col = 0; col < GRID_SIZE; col++) {
				tile_scale[row][col] = 100;
				moved_tiles[row][col] = false;
			}
		}
		animation_step = 0;
	}
}

static void twenty_forty_eight_update(void)
{
	update_tile_animation();
	twenty_forty_eight_state = TWENTY_FORTY_EIGHT_DRAW_SCREEN;
	if (is_game_over()) {
		twenty_forty_eight_state = TWENTY_FORTY_EIGHT_GAME_OVER;
	}
}

static void draw_game_over_screen(void)
{
	char *game_over = "Game Over, man!";
	FbMove(ui_center_text_x(game_over, 0, LCD_XSIZE),
		ui_center_text_y(0, LCD_YSIZE));
	FbWriteString(game_over);
	FbMove(ui_center_text_x(game_over, 0, LCD_XSIZE),
		ui_center_text_y(16, LCD_YSIZE));
	FbWriteString("press B");
	FbMove(ui_center_text_x(game_over, 0, LCD_XSIZE),
		ui_center_text_y(24, LCD_YSIZE));
	FbWriteString("to go back");
}

static void draw_help_screen(void)
{
	FbClear();

	const char *lines[] = {"use the dpad", "to slide tiles", "",
		"tiles of the", "same value will", "combine", "", "create a",
		"tile with", "value of 2048", "to win", "", "",
		"A btn for menu", "B btn for back"};

	int y = 16;
	for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
		y += 8;
		if (lines[i][0] == '\0') {
			/* Skip empty lines for spacing */
			continue;
		}
		FbMove(ui_center_text_x(lines[i], 0, LCD_XSIZE), y);
		FbWriteString(lines[i]);
	}
}

static void draw_game_board(void)
{
	for (int row = 0; row < GRID_SIZE; row++) {
		for (int col = 0; col < GRID_SIZE; col++) {
			int tile_value = get_tile(row, col);
			int x = col * (tile_size + tile_spacing);
			int y = row * (tile_size + tile_spacing);

			int scaled_size =
				(tile_size * tile_scale[row][col]) / 100;

			struct ui_button tile = {
				.x = x + (tile_size - scaled_size) / 2,
				.y = y + (tile_size - scaled_size) / 2,
				.width = scaled_size,
				.height = scaled_size,
				.outline_size = 2,
				.outline_color = palette_color_from_index(
					default_palette, 7),
				.fill_color = tile_value == 6
					? palette_color_from_index(
						  default_palette, 0)
					: palette_color_from_index(
						  default_palette,
						  tile_value + 1),
				.text_color = tile_value == 0
					? palette_color_from_index(
						  default_palette, 1)
					: WHITE,
			};

			ui_button_fill(tile, tile.fill_color);
			ui_button_draw_outline(tile, tile.outline_color);

			if (tile_value > 0) {
				char text[5];
				snprintf(text, sizeof(text), "%d",
					1 << tile_value);
				tile.text = text;
				ui_button_draw_label(tile, tile.text_color);
			}
		}
	}
}

static void draw_screen(void)
{
	FbClear();
	if (twenty_forty_eight_state == TWENTY_FORTY_EIGHT_SHOW_HELP) {
		draw_help_screen();
	}
	if (twenty_forty_eight_state == TWENTY_FORTY_EIGHT_MENU) {
		draw_menu();
	}
	if (twenty_forty_eight_state == TWENTY_FORTY_EIGHT_DRAW_SCREEN) {
		twenty_forty_eight_state = TWENTY_FORTY_EIGHT_RUN;
		draw_game_board();
	}
	if (twenty_forty_eight_state == TWENTY_FORTY_EIGHT_GAME_OVER) {
		draw_game_over_screen();
	}
	FbSwapBuffers();
	screen_changed = 0;
}

void twenty_forty_eight_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (twenty_forty_eight_state) {
	case TWENTY_FORTY_EIGHT_INIT:
		twenty_forty_eight_init();
		break;
	case TWENTY_FORTY_EIGHT_RUN:
		twenty_forty_eight_update();
		break;
	case TWENTY_FORTY_EIGHT_SHOW_HELP:
		draw_screen();
		break;
	case TWENTY_FORTY_EIGHT_DRAW_SCREEN:
		draw_screen();
		break;
	case TWENTY_FORTY_EIGHT_MENU:
		draw_screen();
		break;
	case TWENTY_FORTY_EIGHT_GAME_OVER:
		draw_screen();
		break;
	case TWENTY_FORTY_EIGHT_EXIT:
		twenty_forty_eight_exit();
		break;
	default:
		break;
	}
	check_buttons();
}
