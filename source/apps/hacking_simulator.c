/******************************************************************
 A hacking simulator that's not much like real world hacking at all.

 Author: Dustin Firebaugh <dafirebaugh@gmail.com>
 (c) 2020 Dusitn Firebaugh

 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"

#include "rtc.h"

#include "dynmenu.h"
#include "xorshift.h"

#include "hacking_simulator_drawings/pipes.h"

static int screen_changed = 0;

/* HACKSIM_DEBUG 
 * 1 debug mode on
 * 0 debug mode off
 */
#define HACKSIM_DEBUG 0
#define TRUE 1
#define FALSE 0
#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#define FLOW_RATE 1
#define GRID_SIZE 5
#define CELL_SIZE 20
#define CELL_PADDING 0
#define PIPE_SCALE 480
#define PIPE_OFFSET 10
#define GRID_PADDING 3
#define STARTX(x) (GRID_PADDING + CELL_PADDING + ((x)*CELL_SIZE))
#define ENDX(x) (GRID_PADDING + CELL_SIZE + ((x)*CELL_SIZE))
#define STARTY(y) (GRID_PADDING + CELL_PADDING + ((y)*CELL_SIZE))
#define ENDY(y) (GRID_PADDING + CELL_SIZE + ((y)*CELL_SIZE))
#define WITHIN_GRID(x, y) ((x) >= 0 && (x) <= GRID_SIZE && (y) >= 0 && (y) <= GRID_SIZE - 1)
#define IS_NEIGHBOR(sX, sY, tX, tY) (((sX - 1) == (tX) && (sY) == (tY)) || ((sX - 1) == (tX) && (sY + 1) == (tY)) || ((sX + 1) == (tX) && (sY) == (tY)) || ((sX + 1) == (tX) && (sY - 1) == (tY)) || ((sX) == (tX) && (sY - 1) == (tY)) || ((sX - 1) == (tX) && (sY - 1) == (tY)) || ((sX) == (tX) && (sY + 1) == (tY)) || ((sX + 1) == (tX) && (sY + 1) == (tY)))
#define IS_SOURCE(x,y) ((x) == 0 && (y) == source)
#define HANDX 0
#define HANDY GRID_SIZE
#define TIMERX GRID_SIZE
#define TIMERY GRID_SIZE

/* Difficulty Configuration */
#define CONFIG_DIFFICULTY_INITIAL EASY
#define CONFIG_DIFFICULTY_MIN EASY
#define CONFIG_DIFFICULTY_MAX VERY_HARD
#define CONFIG_DIFFICULTY_DECREASE_AFTER_FAIL TRUE
#define CONFIG_DIFFICULTY_INCREASE_AFTER_WIN TRUE
#define CONFIG_DIFFICULTY_CONSECUTIVE_WINS_TO_ADVANCE 1

#define CONFIG_DIFFICULTY_MODIFIES_TIME 1

/* Time Limit Configuration - all in seconds */
#define CONFIG_GAME_TIME_LIMIT_BASE_S 99
#define CONFIG_GAME_TIME_LIMIT_DIFFICULTY_PENALTY_S 15

#if !CONFIG_DIFFICULTY_MODIFIES_TIME
#undef CONFIG_GAME_TIME_LIMIT_DIFFICULTY_PENALTY_S
#define CONFIG_GAME_TIME_LIMIT_DIFFICULTY_PENALTY_S 0
#endif

/* Program states.  Initial state is HACKINGSIMULATOR_INIT */
enum hacking_simulator_state_t
{
	HACKINGSIMULATOR_INIT,
	HACKINGSIMULATOR_SPLASH_SCREEN,
	HACKINGSIMULATOR_RUN,
	HACKINGSIMULATOR_WIN_SCREEN,
	HACKINGSIMULATOR_FAIL_SCREEN,
	HACKINGSIM_QUIT_CONFIRM,
	HACKINGSIM_QUIT_INPUT,
	HACKINGSIMULATOR_EXIT,
};

static enum hacking_simulator_state_t hacking_simulator_state = HACKINGSIMULATOR_INIT;

enum difficulty_level
{
	EASY = 0,
	MEDIUM = 1,
	HARD = 2,
	VERY_HARD = 3,
};

static char difficulty_descriptor[4][10] = { "EASY", "MEDIUM", "HARD", "VERY HARD" };
static char splash_screen_messages[4][60] = 
{ 
	"Welcome to\nHackingSim\n\nPress button 4\ntimes to quit",
	"Nice Work, It\ngets harder\nnow",
	"Whoa, you\nreally know\nyour stuff.\nLet's make\nit harder",
	"There's no\nway you'll\nbeat this\none",
};

static enum difficulty_level difficulty_level_state = CONFIG_DIFFICULTY_INITIAL;

/* Point Drawings */
static const struct point pipebr_points[] = PIPE_B_R_POINTS;
static const struct point pipebl_points[] = PIPE_B_L_POINTS;
static const struct point pipetr_points[] = PIPE_T_R_POINTS;
static const struct point pipetl_points[] = PIPE_T_L_POINTS;
static const struct point pipetb_points[] = PIPE_T_B_POINTS;
static const struct point pipelr_points[] = PIPE_L_R_POINTS;
static const struct point blocking_square_points[] =
{
	{-18, 18},
	{18, -18},
	{-128, -128},
	{-18, -18},
	{18, 18},
};
static const struct point arrow_points[] =
{
	{0, -9},
	{14, -9},
	{14, -17},
	{26, -3},
	{15, 9},
	{15, 0},
	{0, 0},
	{0, -7},
};

/* Pipe types */
struct pipe_io
{
	int left;
	int right;
	int up;
	int down;
};

struct pipe
{
	int npoints;
	const struct point *drawing;
	const struct pipe_io io;
};

#define INVALID_PIPE_INDEX -2
#define BLOCKING_INDEX -1
#define HORIZONTAL 0
#define VERTICAL 1
#define BOTTOM_RIGHT 2
#define BOTTOM_LEFT 3
#define TOP_RIGHT 4
#define TOP_LEFT 5

static struct pipe pipes[] = {
	{ARRAYSIZE(pipelr_points), pipelr_points, {1, 1, 0, 0}},
	{ARRAYSIZE(pipetb_points), pipetb_points, {0, 0, 1, 1}},
	{ARRAYSIZE(pipebr_points), pipebr_points, {0, 1, 0, 1}},
	{ARRAYSIZE(pipebl_points), pipebl_points, {1, 0, 0, 1}},
	{ARRAYSIZE(pipetr_points), pipetr_points, {0, 1, 1, 0}},
	{ARRAYSIZE(pipetl_points), pipetl_points, {1, 0, 1, 0}},
};

static int cursor_x_index, cursor_y_index;


/* cell_io is used to help determine direction in flow_path */
enum cell_io
{
	NOT_SET,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

struct cell
{
	int x;
	int y;
	int hidden;
	int pipe_index;
	int locked; /* if the water has reached this pipe, it is locked */
	int connected;
	enum cell_io input;
	enum cell_io output;
};

static struct cell grid[GRID_SIZE + 1][GRID_SIZE];

/* hand is used to temporarily hold a pipe that you can swap */
static int hand;
static int hsim_seed;
static int fill_line;
static int source;
static int target;
static int game_tick_s;
static volatile int last_time;
static struct cell *tail;
static struct cell *head;


static int is_fully_connected()
{
	if ((tail->x == GRID_SIZE && tail->y == target) &&
		pipes[tail->pipe_index].io.right)
		return 1;
	return 0;
}

static int is_hand_position(int posX, int posY)
{
	if (posX == HANDX && posY == HANDY)
		return 1;

	return 0;
}

static int is_neighbor(struct cell *current_cell, struct cell *next_cell, struct pipe_io direction)
{
	if (direction.up)
		return current_cell->y-1 == next_cell->y;

	if (direction.down)
		return current_cell->y+1 == next_cell->y;

	if (direction.left)
		return current_cell->x-1 == next_cell->x;
	
	if (direction.right)
		return current_cell->x+1 == next_cell->x;

	return 0;
}

static int io_matches(struct cell *current_cell, struct cell *next_cell)
{
	if (next_cell == NULL)
		return 0;

	if (next_cell->pipe_index == BLOCKING_INDEX)
		return 0;

	struct pipe_io right = {0,1,0,0};
	struct pipe_io left = {1,0,0,0};
	struct pipe_io up = {0,0,1,0};
	struct pipe_io down = {0,0,0,1};

	if (!next_cell->connected && is_neighbor(current_cell, next_cell, right) && pipes[current_cell->pipe_index].io.right && pipes[next_cell->pipe_index].io.left)
		return 1;
	if (!next_cell->connected && is_neighbor(current_cell, next_cell, left) && pipes[current_cell->pipe_index].io.left && pipes[next_cell->pipe_index].io.right)
		return 1;
	if (!next_cell->connected && is_neighbor(current_cell, next_cell, up) && pipes[current_cell->pipe_index].io.up && pipes[next_cell->pipe_index].io.down)
		return 1;
	if (!next_cell->connected && is_neighbor(current_cell, next_cell, down) && pipes[current_cell->pipe_index].io.down && pipes[next_cell->pipe_index].io.up)
		return 1;

	return 0;
}

static struct cell* get_next_cell(struct cell *current_cell);
static int is_cell_blocked(struct cell *current_cell)
{
	struct cell *next_cell = get_next_cell(current_cell);

	if (next_cell == NULL)
		return 1;

	return 0;
}

/* place_blocker places a blocking piece on the grid
*  but we can't have a blocker on the source or the target 
*  or a neighbor of source or target
* and we don't want to overwrite a blocker
*
*/
static void place_blocker()
{
	int x,y;

	int placed_blocker = FALSE;

	do
	{
		hsim_seed++; /* update seed allows us to get a slightly different seed for a different random number */
		x = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;
		y = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;

		/* prevent blocker from being placed on an invalid pipe spot */
		if (grid[x][y].pipe_index != INVALID_PIPE_INDEX)
			continue;

		/* prevent blocker from being placed on the source cell */
		if (x == 0 && y == source)
			continue;

		/* prevent blocker from being placed from being placed on the target cell */
		if (x == GRID_SIZE && y == target)
			continue;

		/* prevent blocker from being placed near the source or target */
		if (IS_NEIGHBOR(0, source, x, y) || IS_NEIGHBOR(GRID_SIZE, target, x, y))
			continue;

		/* prevent blocker from being placed on top of another blocker */
		if (grid[x][y].pipe_index == BLOCKING_INDEX)
			continue;

		placed_blocker = TRUE;
		grid[x][y].pipe_index = BLOCKING_INDEX; /* place blocking sqaure in random spot on grid */
	} while (!placed_blocker);
}

/* ensure_winnable_path determines a winning path and places those pieces on the board */
static void ensure_winnable_path()
{
	int vertical_direction;
	int currentX;
	int currentY = source;

	/* move horizontally */
	for (currentX = 0; currentX < GRID_SIZE; currentX++)
		grid[currentX][currentY].pipe_index = HORIZONTAL;

	/* handle the case where the target is on the same row as the source */
	if (currentY == target)
		grid[currentX][currentY].pipe_index = HORIZONTAL;

	/* move vertically */
	while (currentY != target)
	{
		if (currentX != GRID_SIZE)
			continue;

		if (target > currentY)
		{
			vertical_direction = -1;
			if (currentY == source)
			{ /* handle first elbow on last column */
				grid[currentX][currentY].pipe_index = BOTTOM_LEFT;
			}
			else
			{
				grid[currentX][currentY].pipe_index = VERTICAL;
			}
			currentY++;
			continue;
		}
		else
		{
			vertical_direction = 1;
			if (currentY == source)
			{ /* handle first elbow on last column */
				grid[currentX][currentY].pipe_index = TOP_LEFT;
			}
			else
			{
				grid[currentX][currentY].pipe_index = VERTICAL;
			}
			currentY--;
			continue;
		}
	}

	/* handle the last elbow */
	if (vertical_direction == -1)
		grid[currentX][currentY].pipe_index = TOP_RIGHT;

	if (vertical_direction == 1)
		grid[currentX][currentY].pipe_index = BOTTOM_RIGHT;
}

// TODO: could unroll this for better processing on the microcontroller
static void shuffle()
{
	int i, j, tmpX, tmpY, tmp_pindex;
	int source_neighbor_is_set = FALSE;
	for (i = 0; i <= GRID_SIZE; i++)
	{
		for (j = 0; j < GRID_SIZE; j++)
		{
			tmpX = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;
			tmpY = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;

			/* put a VERTICAL pipe next to the source so that we block the flow at start */
			if (grid[i][j].pipe_index == VERTICAL && !source_neighbor_is_set)
			{
				source_neighbor_is_set = TRUE;
				grid[i][j].pipe_index = grid[1][source].pipe_index;
				grid[1][source].pipe_index = VERTICAL;
			}
			/* don't swap blockers */
			if (grid[i][j].pipe_index == BLOCKING_INDEX || grid[tmpX][tmpY].pipe_index == BLOCKING_INDEX)
			{
				continue;
			}

			/* don't swap the source or the cell next to source
			this ensures that we don't start the flow until 
			the player decides to swap something */
			if ((i == 0 && j == source) || (tmpX == 0 && tmpY == source) ||
				(i == 1 && j == source) || (tmpX == 1 && tmpY == source))
			{
				continue;
			}

			tmp_pindex = grid[i][j].pipe_index;
			grid[i][j].pipe_index = grid[tmpX][tmpY].pipe_index;
			grid[tmpX][tmpY].pipe_index = tmp_pindex;
		}
	}
}

/* place_random_pipe determines the next square to place in 
*  we first place a path that makes winning possible
*  then we place random pipes to fill the grid
*/
static void place_random_pipe(struct cell *cell)
{
	if (cell->pipe_index != INVALID_PIPE_INDEX)
		return;

	cell->pipe_index = xorshift((unsigned int *)&hsim_seed) % ARRAYSIZE(pipes);
}

/* handle difficulty setting
*  we probably shouldn't have more than 3 blockers 
*  because that could potentially block a route from source to target
*/
static void handle_difficulty()
{
	enum difficulty_level i;

	for(i = EASY; i < difficulty_level_state; i++)
		place_blocker();

	game_tick_s = CONFIG_GAME_TIME_LIMIT_BASE_S - CONFIG_GAME_TIME_LIMIT_DIFFICULTY_PENALTY_S * difficulty_level_state;
}

static void modulate_difficulty(unsigned wasWon)
{
	/* 	TODO:	This state should be merged into a clear global game state structure. -PMW */
	static unsigned consecutiveWins = 0;
	if (wasWon)
	{
		consecutiveWins++;
		if ((CONFIG_DIFFICULTY_CONSECUTIVE_WINS_TO_ADVANCE < consecutiveWins)
		&& (CONFIG_DIFFICULTY_MAX > difficulty_level_state))
		{
			consecutiveWins = 0;
			difficulty_level_state += CONFIG_DIFFICULTY_INCREASE_AFTER_WIN;
		}
	}
	else
	{
		consecutiveWins = 0;
		if (CONFIG_DIFFICULTY_MIN < difficulty_level_state)
		{
			difficulty_level_state -= CONFIG_DIFFICULTY_DECREASE_AFTER_FAIL;
		}
	}
}

static void handle_endgame(unsigned wasWon)
{
	modulate_difficulty(wasWon);
	hacking_simulator_state = wasWon ? HACKINGSIMULATOR_WIN_SCREEN : HACKINGSIMULATOR_FAIL_SCREEN;
}

static void initialize_cell(struct cell grid[GRID_SIZE + 1][GRID_SIZE], int x, int y)
{
	if (grid[x][y].pipe_index == BLOCKING_INDEX)
		return;
	grid[x][y].x = x;
	grid[x][y].y = y;
	grid[x][y].hidden = TRUE;
	grid[x][y].connected = FALSE;
	grid[x][y].connected = FALSE;
	/* initialize pipe_index to INVALID_PIPE_INDEX so that we can determine if we have placed a pipe */
	grid[x][y].pipe_index = INVALID_PIPE_INDEX;
}

static void initialize_cells(struct cell grid[GRID_SIZE + 1][GRID_SIZE])
{
	int x, y;
	for (x = 0; x <= GRID_SIZE; x++)
		for (y = 0; y < GRID_SIZE; y++)
			initialize_cell(grid, x, y);
}

static void place_pipe(struct cell grid[GRID_SIZE + 1][GRID_SIZE], int x, int y)
{
	if (grid[x][y].pipe_index == BLOCKING_INDEX && grid[x][y].pipe_index != INVALID_PIPE_INDEX)
		return;
	place_random_pipe(&grid[x][y]);
}

static void place_random_pipes(struct cell grid[GRID_SIZE + 1][GRID_SIZE])
{
	int x, y;
	for (x = 0; x <= GRID_SIZE; x++)
		for (y = 0; y < GRID_SIZE; y++)
			place_pipe(grid, x, y);
}

static void set_source_cell(struct cell *current_cell);

static void initialize_connection()
{
	tail = &grid[0][source];
	set_source_cell(tail);
}

static void set_cell_as_visible(struct cell grid[GRID_SIZE + 1][GRID_SIZE], int source)
{
	grid[0][source].hidden = FALSE;
	grid[0][source].connected = TRUE;
}

static void initialize_grid()
{
	hsim_seed = rtc_get_ms_since_boot();

	/* pick starting point and finishing point -- 
	*  for now we will assume the start is on the left and 
	*  the finish is on the right
	*
	*  adding source to target so that we how often these are on the same row
	*/
	source = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;
	target = (xorshift((unsigned int *)&hsim_seed) + source) % GRID_SIZE;

	initialize_cells(grid);

	/* TODO: it might make it more difficult if we place blockers and then find the win-path */
	ensure_winnable_path();
	handle_difficulty();
	place_random_pipes(grid);
	set_cell_as_visible(grid, source);
	initialize_connection();
	shuffle();
	hand = 0;
}

static void set_source_cell(struct cell *current_cell)
{
	current_cell->connected = TRUE;
	current_cell->output = RIGHT;
	head = current_cell;
}

static struct cell* get_cell(int x, int y)
{
	if (!WITHIN_GRID(x, y))
		return NULL;

	return &grid[x][y];
}

static struct cell* get_next_cell(struct cell *current_cell)
{
	/* try all four neighbors 
	 * we would have to rule out connected */
	int x = current_cell->x;
	int y = current_cell->y;
	struct point up_neighbor_coordinate = {
		x,
		y-1,
	};
	struct point down_neighbor_coordinate = {
		x,
		y+1,
	};
	struct point left_neighbor_coordinate = {
		x-1,
		y,
	};
	struct point right_neighbor_coordinate = {
		x+1,
		y,
	};

	if (io_matches(current_cell, get_cell(right_neighbor_coordinate.x, right_neighbor_coordinate.y)))
		return &grid[right_neighbor_coordinate.x][right_neighbor_coordinate.y];
	if (io_matches(current_cell, get_cell(up_neighbor_coordinate.x, up_neighbor_coordinate.y)))
		return &grid[up_neighbor_coordinate.x][up_neighbor_coordinate.y];
	if (io_matches(current_cell, get_cell(down_neighbor_coordinate.x, down_neighbor_coordinate.y)))
		return &grid[down_neighbor_coordinate.x][down_neighbor_coordinate.y];
	if (io_matches(current_cell, get_cell(left_neighbor_coordinate.x, left_neighbor_coordinate.y)))
		return &grid[left_neighbor_coordinate.x][left_neighbor_coordinate.y];

	return NULL;
}

static void check_for_win()
{
	if (!is_fully_connected())
		return;

	if (difficulty_level_state < VERY_HARD) {
		difficulty_level_state++;
		hacking_simulator_state = HACKINGSIMULATOR_INIT;
		return;
	}
	handle_endgame(TRUE);
}

static void check_for_loss()
{
	/*
	 *	Ultimately if the player is out of time we don't need to check anything
	 *  else, so we can safely return here. -PMW
	 */
	if (game_tick_s <= 0)
	{
		handle_endgame(FALSE);
		return;
	}

	struct cell *next_cell = get_next_cell(tail);
	if (next_cell == NULL)
		return;

	if (!WITHIN_GRID(next_cell->x, next_cell->y))
		handle_endgame(FALSE);
}

static void advance_tail()
{
	if (is_cell_blocked(tail))
		return;

	tail->connected = TRUE;

	struct cell* next_cell = get_next_cell(tail);

	if (next_cell == NULL)
		return;

	tail = next_cell;
}

static void evaluate_connection()
{
	check_for_win();
	check_for_loss();
	advance_tail();
}


static int get_time(void)
{
    return (int) rtc_get_time_of_day().tv_sec;
}

static void advance_tick()
{	
	int current_time = get_time();
	if ((current_time % 60) != (last_time % 60))
	{
		if (game_tick_s > 0)
		{
			last_time = current_time;
			game_tick_s--;
			screen_changed = 1;
			return;
		}

		/* endgame */
		return;
	}
}

static void swap_with_hand(struct cell *cell, int tmp_pipe)
{
	cell->output = NOT_SET;
	cell->input = NOT_SET;
	hand = cell->pipe_index;
	cell->pipe_index = tmp_pipe;
	tail = head;
	evaluate_connection();
}

static void reset_flow_connections()
{
	int i, j;
	for (i = 0; i <= GRID_SIZE; i++)
		for (j = 0; j < GRID_SIZE; j++)
			grid[i][j].connected = FALSE;

	grid[0][source].connected = TRUE;
}

static void render_screen();
static void check_buttons()
{
	static int button_presses = 0;

    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
	{
		button_presses++;

		if (grid[cursor_x_index][cursor_y_index].hidden)
		{
			grid[cursor_x_index][cursor_y_index].hidden = FALSE;
		}
		else
		{
			if (grid[cursor_x_index][cursor_y_index].pipe_index == BLOCKING_INDEX)
				return;
			if (grid[cursor_x_index][cursor_y_index].connected)
				return;

			swap_with_hand(&grid[cursor_x_index][cursor_y_index], hand);
			reset_flow_connections();
		}
		screen_changed = 1;
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches))
	{
		button_presses = 0;
		cursor_x_index--;
		screen_changed = 1;
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
	{
		button_presses = 0;
		cursor_x_index++;
		screen_changed = 1;
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
	{
		button_presses = 0;
		cursor_y_index--;
		screen_changed = 1;
	}
	else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches))
	{
		button_presses = 0;
		cursor_y_index++;
		screen_changed = 1;
	}

	if (cursor_x_index < 0)
		cursor_x_index = 0;
	if (cursor_x_index > GRID_SIZE)
		cursor_x_index = GRID_SIZE;
	if (cursor_y_index < 0)
		cursor_y_index = 0;
	if (cursor_y_index > GRID_SIZE - 1)
		cursor_y_index = GRID_SIZE - 1;

	render_screen();

	if (button_presses == 4) { /* Trying to quit? */
		button_presses = 0;
		hacking_simulator_state = HACKINGSIM_QUIT_CONFIRM;
	}
}

static void handle_splash_screen_btn()
{
	initialize_grid();
	hacking_simulator_state = HACKINGSIMULATOR_RUN;
}

static void render_flow_cell(struct cell current_cell)
{
	int i;
	for (i = 0; i < CELL_SIZE; i++)
		FbHorizontalLine(STARTX(current_cell.x), ENDY(current_cell.y) - i, ENDX(current_cell.x), 0);
}

static void render_pipe_fill(struct cell cell)
{
	FbColor(BLUE);
	render_flow_cell(cell);
}

static void render_splash_screen()
{
	FbColor(WHITE);
	FbMove(0, 20);
	FbWriteString(splash_screen_messages[difficulty_level_state]);
	FbSwapBuffers();
}

static void render_hackingsimulator_splash_screen()
{
	render_splash_screen();

    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
		handle_splash_screen_btn();
}

static void render_hackingsimulator_fail_screen()
{
	FbClear();
	FbColor(WHITE);
	FbMove(0, 20);
	FbWriteString("You FAILED to\ncomplete the\npuzzle in\nthe allotted\ntime.");
	FbSwapBuffers();

    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
		hacking_simulator_state = HACKINGSIMULATOR_EXIT;
}

static void render_hackingsimulator_win_screen()
{
	FbClear();
	FbColor(WHITE);
	FbMove(0, 20);
	FbWriteLine("You WON!");
	FbSwapBuffers();

    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches))
		hacking_simulator_state = HACKINGSIMULATOR_EXIT;
}

#define VERY_HARD_FRAMES_TO_RENDER 45
#define VERY_HARD_FRAMES_TO_HIDE 25
static int frame_count = 0;
static int is_drawing_grid = 0;

static void toggle_grid_render()
{
	frame_count = 0;

	if (is_drawing_grid)
	{
		is_drawing_grid = 0;
		return;
	}

	is_drawing_grid = 1;
}

static void render_grid();
static void render_cursor();
static void handle_flicker()
{
	frame_count++;

	if (is_drawing_grid && frame_count == VERY_HARD_FRAMES_TO_RENDER)
		toggle_grid_render();

	if (!is_drawing_grid && frame_count == VERY_HARD_FRAMES_TO_HIDE)
		toggle_grid_render();

	if (is_drawing_grid)
		render_grid();

	render_cursor();
	FbSwapBuffers();
}

static void render_blocking_cell(struct cell cell)
{
	FbDrawObject(blocking_square_points, ARRAYSIZE(blocking_square_points), RED, STARTX(cell.x) + PIPE_OFFSET, STARTY(cell.y) + PIPE_OFFSET, PIPE_SCALE);
}

static void render_pipe(struct cell cell)
{
	/* Check to see if the cell is blocking */
	if (cell.pipe_index == BLOCKING_INDEX)
	{
		render_blocking_cell(cell);
		return;
	}

	if (cell.connected)
		render_pipe_fill(cell);

	FbDrawObject(pipes[cell.pipe_index].drawing, pipes[cell.pipe_index].npoints, YELLOW, STARTX(cell.x) + PIPE_OFFSET, STARTY(cell.y) + PIPE_OFFSET, PIPE_SCALE);
#ifdef HACKSIM_DEBUG
	if (cell.connected)
		FbMove(STARTX(cell.x), STARTY(cell.y));
#endif
	FbColor(WHITE);
}

static void render_hand()
{
	/* Check to see if the cell is blocking */
	FbDrawObject(pipes[hand].drawing, pipes[hand].npoints, YELLOW, STARTX(HANDX) + PIPE_OFFSET, STARTY(HANDY) + PIPE_OFFSET, PIPE_SCALE);
	FbColor(WHITE);
}

static void render_timer()
{
	char p[4];
	FbMove(STARTX(TIMERX) + (CELL_SIZE / 5), STARTY(TIMERY) + (CELL_SIZE / 2));
	sprintf( p, "%d", game_tick_s);
	FbWriteLine(p);
}

static void render_box(int grid_x, int grid_y, int color)
{
	FbColor(color);
	FbHorizontalLine(STARTX(grid_x), STARTY(grid_y), ENDX(grid_x), STARTY(grid_y));
	FbHorizontalLine(STARTX(grid_x), ENDY(grid_y), ENDX(grid_x), ENDY(grid_y));
	FbVerticalLine(STARTX(grid_x), STARTY(grid_y), STARTX(grid_x), ENDY(grid_y));
	FbVerticalLine(ENDX(grid_x), STARTY(grid_y), ENDX(grid_x), ENDY(grid_y));
}

static void render_arrow(int x, int y)
{
	FbDrawObject(arrow_points, ARRAYSIZE(arrow_points), GREEN, x + PIPE_OFFSET, y + PIPE_OFFSET, PIPE_SCALE);
}

static void render_cell(int posX, int posY)
{
#if HACKSIM_DEBUG
	char p[4];
#endif

	if (is_hand_position(posX, posY))
	{
		render_box(posX, posY, BLUE);
		render_hand();
		render_timer();
#if HACKSIM_DEBUG
		FbMove(STARTX(posX) + (CELL_SIZE / 5), STARTY(posY) + (CELL_SIZE / 2));
		sprintf( current_cell->input, "%d", p);
		FbWriteLine(p);
#endif
		return;
	}

	render_box(posX, posY, WHITE);

	if (grid[posX][posY].hidden == 1)
	{
		FbMove(STARTX(posX) + (CELL_SIZE / 2), STARTY(posY) + (CELL_SIZE / 2));
#if HACKSIM_DEBUG
		FbWriteLine("?");
#endif
	}
	else
	{
		render_pipe(grid[posX][posY]);

#if HACKSIM_DEBUG
		FbMove(STARTX(posX) + (CELL_SIZE / 5), STARTY(posY) + (CELL_SIZE / 2));
		sprintf( grid[posX][posY].pipe_index, "%d", p);
		FbWriteLine(p);
#endif
	}

	if (posX == 0 && posY == source)
		render_arrow(STARTX(0) - 10, STARTY(posY));

	if (posX == GRID_SIZE && posY == target)
		render_arrow(STARTX(GRID_SIZE), STARTY(posY));
}

static void render_hand_cell()
{
	FbColor(BLUE);
	render_cell(HANDX, HANDY);
}

static void render_grid()
{
	int x, y;

	for (x = 0; x <= GRID_SIZE; x++)
		for (y = 0; y < GRID_SIZE; y++)
			render_cell(x, y);

	render_hand_cell();
}

static void render_cursor()
{
	render_box(cursor_x_index, cursor_y_index, GREEN);
}

static void render_difficulty_label()
{
	FbColor(RED);
	FbMove(30, 110);
	FbWriteLine(difficulty_descriptor[difficulty_level_state]);
}

static void render_screen()
{
	if (!screen_changed)
		return;

	FbColor(WHITE);
	FbClear();
	render_difficulty_label();

	if (difficulty_level_state == VERY_HARD)
	{
		handle_flicker();
		return;
	}

	render_grid();
	render_cursor();
	FbSwapBuffers();
	screen_changed = 0;
}

static void hackingsimulator_init()
{
	FbInit();
	FbClear();
	cursor_x_index = 0;
	cursor_y_index = 0;
	fill_line = 0;
	hacking_simulator_state = HACKINGSIMULATOR_SPLASH_SCREEN;
}

static void hackingsimulator_run()
{
	check_buttons();
	advance_tick();
	evaluate_connection();
}

static void hackingsimulator_exit()
{
	hacking_simulator_state = HACKINGSIMULATOR_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

static struct dynmenu quitmenu;
static struct dynmenu_item quitmenu_item[2];

static void hackingsimulator_quit_confirm(void)
{
	dynmenu_init(&quitmenu, quitmenu_item, ARRAYSIZE(quitmenu_item));
	dynmenu_clear(&quitmenu);
	strcpy(quitmenu.title, "HACKING SIM");
	strcpy(quitmenu.title2, "REALLY QUIT?");
	dynmenu_add_item(&quitmenu, "NO", HACKINGSIMULATOR_RUN, 0);
	dynmenu_add_item(&quitmenu, "YES", HACKINGSIMULATOR_EXIT, 0);
	quitmenu.menu_active = 1;
	dynmenu_draw(&quitmenu);
	FbSwapBuffers();
	hacking_simulator_state = HACKINGSIM_QUIT_INPUT;
}

static void hackingsimulator_quit_input(void)
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		hacking_simulator_state =
			(enum hacking_simulator_state_t) quitmenu.item[quitmenu.current_item].next_state;
		return;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		dynmenu_change_current_selection(&quitmenu, -1);
		dynmenu_draw(&quitmenu);
		FbSwapBuffers();
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		dynmenu_change_current_selection(&quitmenu, 1);
		dynmenu_draw(&quitmenu);
		FbSwapBuffers();
	}
}

int hacking_simulator_cb(void)
{
	switch (hacking_simulator_state)
	{
	case HACKINGSIMULATOR_INIT:
		hackingsimulator_init();
		break;
	case HACKINGSIMULATOR_SPLASH_SCREEN:
		render_hackingsimulator_splash_screen();
		break;
	case HACKINGSIMULATOR_WIN_SCREEN:
		render_hackingsimulator_win_screen();
		break;
	case HACKINGSIMULATOR_RUN:
		hackingsimulator_run();
		break;
	case HACKINGSIMULATOR_FAIL_SCREEN:
		render_hackingsimulator_fail_screen();
		break;
	case HACKINGSIMULATOR_EXIT:
		hackingsimulator_exit();
		break;
	case HACKINGSIM_QUIT_CONFIRM:
		hackingsimulator_quit_confirm();
		break;
	case HACKINGSIM_QUIT_INPUT:
		hackingsimulator_quit_input();
		break;
	default:
		break;
	}
	return 0;
}

