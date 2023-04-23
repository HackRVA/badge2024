#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "badge.h"
#include "dynmenu.h"
#include "random.h"
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "xorshift.h"
#include "ir.h"
#include "audio.h"
#include "init.h"
#include "clue_assets.h"
#include "rtc.h"

#define BADGE_IR_CLUE_GAME_ADDRESS IR_APP3
#define BADGE_IR_BROADCAST_ID 0
#define CLUESOE ((uint64_t) 0xC100050E << 32)

#define DEBUG_UDP 1
#if DEBUG_UDP
#define udpdebug fprintf
#else
#define udpdebug(...) { }
#endif

#ifdef TARGET_PICO
/* printf?  What printf? */
#define printf(...) { }
#endif

/* These need to be protected from interrupts. */
#define QUEUE_SIZE 5
#define QUEUE_DATA_SIZE 10
static int queue_in;
static int queue_out;
static IR_DATA packet_queue[QUEUE_SIZE] = { {0} };
static uint8_t packet_data[QUEUE_SIZE][QUEUE_DATA_SIZE] = {{0}};
static int scan_for_incoming_packets = 0;
static uint64_t start_time_ms_since_boot = 0;
static uint64_t final_time_ms_since_boot = 0;
static uint64_t clue_run_idle_time = 0;

static int game_in_progress = 0;
static signed char playing_as_character = -1;
static int questions_asked = 0;
static unsigned int clue_random_seed = 0;
static int screen_changed = 0;

/* Program states.  Initial state is CLUE_INIT */
enum clue_state_t {
	CLUE_INIT,
	CLUE_INIT_MAIN_MENU,
	CLUE_MAIN_MENU,
	CLUE_NEW_GAME,
	CLUE_RUN,
	CLUE_NOTEBOOK,
	CLUE_EVIDENCE,
	CLUE_INTERVIEW,
	CLUE_RECEIVED_ANSWER,
	CLUE_CONFIRM_ACCUSE,
	CLUE_ACCUSE,
	CLUE_CHECK_ACCUSATION,
	CLUE_TRANSMIT_QUESTION,
	CLUE_TRANSMIT_ANSWER,
	CLUE_LUCKY,
	CLUE_CLEVER,
	CLUE_WRONG_ACCUSATION,
	CLUE_EXIT,
};

enum card_type {
	CARD_TYPE_WEAPON,
	CARD_TYPE_ROOM,
	CARD_TYPE_SUSPECT,
};

struct card {
	char *name;
	char *short_name;
	enum card_type type;
	const struct asset *pic;
};

struct notebook {
	unsigned char k[21][6];
};
static struct notebook notebook = { 0 };

struct evidence {
	unsigned char from_who[21];
};

static struct evidence evidence = { 0 };

static struct question {
	int person, location, weapon;
} question = { 0 };

#define NSUSPECTS 6
#define NLOCATIONS 9
#define NWEAPONS 6

static const struct card card[] = {
	{ "Miss Scarlett", "Scarlett", CARD_TYPE_SUSPECT, &clue_assets_miss_scarlett, },
	{ "Colonel Mustard", "Mustard", CARD_TYPE_SUSPECT, &clue_assets_col_mustard, },
	{ "Mrs. White", "White", CARD_TYPE_SUSPECT, &clue_assets_ms_white, },
	{ "Mr. Green", "Green", CARD_TYPE_SUSPECT, &clue_assets_mr_green, },
	{ "Mrs. Peacock", "Peacock", CARD_TYPE_SUSPECT, &clue_assets_mrs_peacock, },
	{ "Professor Plum", "Plum", CARD_TYPE_SUSPECT, &clue_assets_prof_plum, },

	{ "Kitchen", "Kitch", CARD_TYPE_ROOM, &clue_assets_kitchen, },
	{ "Ballroom", "Ballrm", CARD_TYPE_ROOM, &clue_assets_ballroom, },
	{ "Conservatory", "Consv", CARD_TYPE_ROOM, &clue_assets_conservatory, },
	{ "Billiard Room", "Billrd", CARD_TYPE_ROOM, &clue_assets_billiards_room, },
	{ "Library", "Library", CARD_TYPE_ROOM, &clue_assets_library, },
	{ "Study", "Study", CARD_TYPE_ROOM, &clue_assets_study, },
	{ "Hall", "Hall", CARD_TYPE_ROOM, &clue_assets_hall, },
	{ "Lounge", "Lounge", CARD_TYPE_ROOM, &clue_assets_lounge, },
	{ "Dining Room", "Dining", CARD_TYPE_ROOM, &clue_assets_dining_room, },

	{ "Rope", "Rope", CARD_TYPE_WEAPON, &clue_assets_rope, },
	{ "Knife", "Knife", CARD_TYPE_WEAPON, &clue_assets_knife, },
	{ "Wrench", "Wrench", CARD_TYPE_WEAPON, &clue_assets_wrench, },
	{ "Revolver", "Rvlvr", CARD_TYPE_WEAPON, &clue_assets_revolver, },
	{ "Candlestick", "Cndlstk", CARD_TYPE_WEAPON, &clue_assets_candlestick, },
	{ "Lead Pipe", "Pipe", CARD_TYPE_WEAPON, &clue_assets_lead_pipe, },
};

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#define NCARDS (int) ARRAYSIZE(card)

struct deck {
	unsigned char card[ARRAYSIZE(card)];
	unsigned char held_by[ARRAYSIZE(card)];
};

static struct deck current_deck = { 0 };

static unsigned int random_num_state = 0;
static int question_answer = -1;
static int new_evidence = -1;

static void init_random_state(void)
{
	random_insecure_bytes((uint8_t *) &random_num_state, sizeof(random_num_state));
	clue_random_seed = random_num_state; /* save this for later use */
}

/* return a random int between 0 and n - 1 */
static int random_num(int n)
{
	int x;

	assert(n != 0);
	x = xorshift(&random_num_state);
	if (x < 0)
		x = -x;
	return x % n;
}

static void deal_cards(unsigned int seed, struct deck *d)
{
	int murderer, murder_weapon, murder_location;

	for (int i = 0; i < NCARDS; i++) {
		d->card[i] = i;
		d->held_by[i] = 0;
	}

	random_num_state = seed;

	/* Set up the crime */
	murderer = random_num(NSUSPECTS);
	murder_location = random_num(NLOCATIONS) + NSUSPECTS;
	murder_weapon = random_num(NWEAPONS) + NSUSPECTS + NLOCATIONS;

	d->held_by[murderer] = 255;
	d->held_by[murder_location] = 255;
	d->held_by[murder_weapon] = 255;
#if 0
	printf("It was %s\nin the %s\nwith the %s!\n",
		card[murderer].name, card[murder_location].name, card[murder_weapon].name);
#endif

	/* Shuffle the deck */
	for (int i = 0; i < NCARDS; i++) {
		int n = random_num(NCARDS);
		unsigned char tc, th;
		tc = d->card[i];
		th = d->held_by[i];
		d->card[i] = d->card[n];
		d->held_by[i] = d->held_by[n];
		d->card[n] = tc;
		d->held_by[n] = th;
	}

	/* Deal the cards to the suspects */
	int suspect = 0;
	for (int i = 0; i < NCARDS; i++) {
		if (d->held_by[i] == 255) /* don't deal the murder cards */
			continue;
		d->held_by[i] = suspect;
		suspect = (suspect + 1) % NSUSPECTS;
	}
}

static enum clue_state_t clue_state = CLUE_INIT;

static void change_clue_state(enum clue_state_t new_state)
{
	screen_changed = 1;
	clue_state = new_state;
}

static struct dynmenu main_menu;
static struct dynmenu_item menu_item[15];
static struct dynmenu game_menu;
static struct dynmenu_item game_menu_item[15];

static void clue_ir_packet_callback(const IR_DATA *data)
{
	// This is called in an interrupt context!
	int next_queue_in;

	next_queue_in = (queue_in + 1) % QUEUE_SIZE;
	if (next_queue_in == queue_out) /* queue is full, drop packet */
		return;
	size_t data_size = data->data_length;
	if (QUEUE_DATA_SIZE < data_size) {
		data_size = QUEUE_DATA_SIZE;
	}
	memcpy(&packet_data[queue_in], data->data, data_size);
	memcpy(&packet_queue[queue_in], data, sizeof(packet_queue[0]));
	packet_queue[queue_in].data = packet_data[queue_in];

	queue_in = next_queue_in;
}

static void clue_init(void)
{
	FbInit();
	FbClear();
	change_clue_state(CLUE_INIT_MAIN_MENU);
	screen_changed = 1;
	playing_as_character = (signed char) badge_system_data()->badgeId & 0x0F;
	playing_as_character = playing_as_character % 6;
	question.person = -1;
	question.location = -1;
	question.weapon = -1;
	ir_add_callback(clue_ir_packet_callback, BADGE_IR_CLUE_GAME_ADDRESS);
	scan_for_incoming_packets = 1;
}

static void clue_init_main_menu(void)
{
	dynmenu_init(&main_menu, menu_item, ARRAYSIZE(menu_item));
	dynmenu_clear(&main_menu);
	strcpy(main_menu.title, "CLUE");
	if (game_in_progress)
		dynmenu_add_item(&main_menu, "RESUME GAME", CLUE_RUN, 0);
	dynmenu_add_item(&main_menu, "START NEW GAME", CLUE_NEW_GAME, 1);
	if (game_in_progress)
		dynmenu_add_item(&main_menu, "PAUSE_GAME", CLUE_EXIT, 2);
	else
		dynmenu_add_item(&main_menu, "QUIT", CLUE_EXIT, 2);

	dynmenu_init(&game_menu, game_menu_item, ARRAYSIZE(game_menu_item));
	dynmenu_clear(&game_menu);
	strcpy(game_menu.title, "CLUE");
	strcpy(game_menu.title2, "MY NAME IS:");
	strcpy(game_menu.title3, card[playing_as_character].name);
	dynmenu_add_item(&game_menu, "NOTEBOOK", CLUE_NOTEBOOK, 0);
	dynmenu_add_item(&game_menu, "EVIDENCE", CLUE_EVIDENCE, 1);
	dynmenu_add_item(&game_menu, "QSTN SUSPECT", CLUE_INTERVIEW, 2);
	dynmenu_add_item(&game_menu, "ACCUSE", CLUE_CONFIRM_ACCUSE, 3);
	dynmenu_add_item(&game_menu, "PAUSE GAME", CLUE_EXIT, 4);

	change_clue_state(CLUE_MAIN_MENU);
}

static void draw_elapsed_time(void)
{
	static int last_second = -1;
	static char msg[15] = "TIME:         ";
	static char lastmsg[15];
	uint64_t current_time = rtc_get_ms_since_boot();
	if (final_time_ms_since_boot != 0)
		current_time = final_time_ms_since_boot;
	uint64_t elapsed_time_ms = current_time - start_time_ms_since_boot;
	uint64_t elapsed_time_s = elapsed_time_ms / 1000;
	uint64_t elapsed_time_min = elapsed_time_s / 60;
	uint64_t elapsed_time_hours = elapsed_time_min / 60;


	int hours = (int) elapsed_time_hours;
	int mins = (int) (elapsed_time_min - (elapsed_time_hours * 60));
	int secs = (int) (elapsed_time_s - (elapsed_time_hours * 3600) - (elapsed_time_min * 60));
	if (last_second != secs) {
		last_second = secs;
		screen_changed = 1;
	}

	memset(msg, 0, sizeof(msg));
	snprintf(msg, sizeof(msg), "TIME: %02d:%02d:%02d", hours, mins, secs);
	FbColor(YELLOW);
	FbBackgroundColor(BLACK);
	FbMove(8, 16 * 8);
	FbWriteString(msg);
	memcpy(lastmsg, msg, sizeof(lastmsg));
}

static void draw_question_count(void)
{
	char msg[15];
	FbColor(YELLOW);
	FbBackgroundColor(BLACK);
	FbMove(8, 17 * 8);
	snprintf(msg, sizeof(msg), "QUESTIONS: %d\n", questions_asked);
	FbWriteString(msg);
}

static void update_screen(void)
{
	if (screen_changed)
		FbSwapBuffers();
	screen_changed = 0;
}

static void clue_main_menu(void)
{
	dynmenu_draw(&main_menu);
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		dynmenu_change_current_selection(&main_menu, 1);
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		dynmenu_change_current_selection(&main_menu, -1);
		screen_changed = 1;
		
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		change_clue_state(main_menu.item[main_menu.current_item].next_state);
	}
}

static void suppress_screensaver(void)
{
	button_reset_last_input_timestamp();
}

static void clue_run()
{
	static int idle = 0;
	FbClear();
	uint64_t current_time = rtc_get_ms_since_boot();
	if (((current_time - clue_run_idle_time) / 1000) > 30) {
		if (!idle)
			screen_changed = 1;
		idle = 1;
		FbMove(0, 0);
		FbWriteString("MY NAME IS:\n");
		FbWriteString(card[playing_as_character].name);
		FbMove(14, 30);
		FbImage(card[playing_as_character].pic, 0);
		suppress_screensaver();
	} else {
		if (idle)
			screen_changed = 1;
		idle = 0;
		dynmenu_draw(&game_menu);
		draw_elapsed_time();
		draw_question_count();
	}
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		if (!idle)
			dynmenu_change_current_selection(&game_menu, 1);
		clue_run_idle_time = rtc_get_ms_since_boot();
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		if (!idle)
			dynmenu_change_current_selection(&game_menu, -1);
		clue_run_idle_time = rtc_get_ms_since_boot();
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		if (!idle)
			change_clue_state(game_menu.item[game_menu.current_item].next_state);
		clue_run_idle_time = rtc_get_ms_since_boot();
	}
}

static void clue_exit()
{
	change_clue_state(CLUE_INIT); /* So that when we start again, we do not immediately exit */
	ir_remove_callback(clue_ir_packet_callback, BADGE_IR_CLUE_GAME_ADDRESS);
	scan_for_incoming_packets = 0;
	returnToMenus();
}

static void write_suspect_initial(int x, int y, int color, char *initial)
{
	FbMove(x, y);
	FbColor(color);
	FbWriteString(initial);
}

static void write_notebook_header(void)
{
	int x = 128 - (6 * 8);
	int y = 1;
	write_suspect_initial(x, y, RED, "S"); x += 8;
	write_suspect_initial(x, y, YELLOW, "M"); x += 8;
	write_suspect_initial(x, y, WHITE, "W"); x += 8;
	write_suspect_initial(x, y, GREEN, "G"); x += 8;
	write_suspect_initial(x, y, CYAN, "P"); x += 8;
	write_suspect_initial(x, y, MAGENTA, "P"); x += 8;
}

static void set_cursor_color(int color, int invert)
{
	if (invert) {
		FbColor(BLACK);
		FbBackgroundColor(color);
	} else {
		FbColor(color);
		FbBackgroundColor(BLACK);
	}
}

static void draw_notebook(int top_item, int row, int col)
{
	int x, y;

	FbClear();
	write_notebook_header();
	for (int i = 0; i < NCARDS; i++) {
		if (i < top_item)
			continue;
		y = 9 + (i - top_item) * 9;
		if (y > 155)
			break;
		x = 1;
		FbMove(x, y);
		if (i == row) {
			FbColor(BLACK);
			FbBackgroundColor(WHITE);
		} else {
			FbColor(x11_gray);
			FbBackgroundColor(BLACK);
		}
		FbWriteString(card[i].short_name);
		FbBackgroundColor(BLACK);
		FbColor(WHITE);
		for (int j = 0; j < 6; j++) {
			int invert = (col == j && row == i);
			x = 120 - 40 + j * 8;
			switch (notebook.k[i][j]) {
			case '?':
				set_cursor_color(x11_gray, invert);
				break;
			case '+':
				set_cursor_color(GREEN, invert);
				break;
			case 'X':
				set_cursor_color(RED, invert);
				break;
			case 'o':
				set_cursor_color(YELLOW, invert);
				break;
			default:
				set_cursor_color(WHITE, invert);
				break;
			}
			FbMove(x, y);
			char c[2];
			c[0] = notebook.k[i][j];
			c[1] = '\0';
			FbWriteString(c);
		}
	}
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
}

static void clue_notebook()
{
	static int top_item = 0;;
	static int row = 0;
	static int col = 0;

	FbClear();
	draw_notebook(top_item, row, col);
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		row++;
		if (row > 20)
			row = 20;
		if (row > top_item + 15) {
			top_item++;
			if (top_item > 11)
				top_item = 11;
		}
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		row--;
		if (row < 0)
			row = 0;
		if (row < top_item)
			top_item = row;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		col++;
		if (col > 5)
			col = 5;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		col--;
		if (col < 0)
			col = 0;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		unsigned char nc = '?';
		switch (notebook.k[row][col]) {
		case '?':
			nc = '+';
			break;
		case '+':
			nc = 'X';
			break;
		case 'X':
			nc = 'o';
			break;
		case 'o':
			nc = '?';
			break;
		default:
			nc = '?';
		}
		notebook.k[row][col] = nc;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		clue_run_idle_time = rtc_get_ms_since_boot();
		change_clue_state(CLUE_RUN);
	}
}

static void clue_evidence(void)
{
	char msg[20];
	static int n = 0;
	int ni = 0;
	FbClear();
	int count = 0;

	FbClear();
	FbColor(WHITE);
	FbBackgroundColor(BLACK);

	ni = -1;
	for (int i = 0; i < NCARDS; i++) {
		if (evidence.from_who[i] != 255) {
			count++;
			if (count == n + 1)
				ni = i;
		}
	}

	if (new_evidence >= 0)
		ni = new_evidence;

	FbWriteString("EVIDENCE\n");
	if (ni < 0) {
		FbWriteString("No Evidence\n");
	} else {
		if (evidence.from_who[ni] != 255) {
			snprintf(msg, sizeof(msg), "ITEM %d of %d\n\n", n + 1, count);
			FbMove(14, 23);
			FbImage(card[ni].pic, 0);
			FbMove(1, 0);
			FbWriteString(msg);
			FbWriteString("ELIMINATED:\n");
			FbMove(6, 123);
			FbColor(YELLOW);
			FbWriteString(card[ni].name);
			FbColor(WHITE);
			FbMove(1, LCD_YSIZE - 8 * 3);
			FbWriteString("INFORMATION\nPROVIDED BY\n");
			FbColor(YELLOW);
			FbWriteString(card[evidence.from_who[ni]].name);
			FbColor(WHITE);
		}
	}
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		new_evidence = -1;
		n++;
		if (n > count - 1)
			n = count - 1;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		new_evidence = -1;
		n--;
		if (n < 0)
			n = 0;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		new_evidence = -1;
		clue_run_idle_time = rtc_get_ms_since_boot();
		change_clue_state(CLUE_RUN);
	}
}

static void clue_interview(int making_accusation)
{
	static int n = 0;
	FbClear();

	FbMove(1, 1);
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	if (!making_accusation) {
		FbWriteString("QUESTION:\n\n");
		FbWriteString("I THINK\n");
	} else {
		FbWriteString("I ACCUSE\n");
	}
	FbColor(n == 0 ? GREEN : YELLOW);
	if (question.person == -1) {
		FbWriteString("PERSON\n");
	} else {
		FbWriteString(card[question.person].name);
		FbMoveX(0);
		FbWriteString("\n");
	}
	FbColor(WHITE);
	if (!making_accusation)
		FbWriteString("\nDID IT IN THE\n");
	else
		FbWriteString("\nOF MURDER\nIN THE\n");
	FbColor(n == 1 ? GREEN : YELLOW);
	if (question.location == -1) {
		FbWriteString("LOCATION\n");
	} else {
		FbWriteString(card[question.location].name);
		FbMoveX(0);
		FbWriteString("\n");
	}
	FbColor(WHITE);
	FbWriteString("\nWITH THE\n");
	FbColor(n == 2 ? GREEN : YELLOW);
	if (question.weapon == -1) {
		FbWriteString("WEAPON\n");
	} else {
		FbWriteString(card[question.weapon].name);
		FbMoveX(0);
		FbWriteString("\n");
	}
	FbColor(n == 3 ? GREEN : YELLOW);
	if (making_accusation) 
		FbWriteString("\n\nMAKE ACCUSATION");
	else
		FbWriteString("\n\nASK QUESTION");
	FbColor(WHITE);
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		n++;
		if (n > 3)
			n = 3;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		n--;
		if (n < 0)
			n = 0;
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		switch (n) {
		case 0:
			question.person--;
			if (question.person < 0)
				question.person = 5;
			break;
		case 1:
			question.location--;
			if (question.location < 6)
				question.location = 6 + 9 - 1;
			break;
		case 2:
			question.weapon--;
			if (question.weapon < 6 + 9)
				question.weapon = 6 + 6 + 9 - 1;
			break;
		default:
			break;
		}
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		switch (n) {
		case 0:
			question.person++;
			if (question.person > 5)
				question.person = 0;
			break;
		case 1:
			question.location++;
			if (question.location > 6 + 9 - 1)
				question.location = 6;
			if (question.location < 6)
				question.location = 6;
			break;
		case 2:
			question.weapon++;
			if (question.weapon > 6 + 6 + 9 - 1)
				question.weapon = 6 + 9;
			if (question.weapon < 6 + 9)
				question.weapon = 6 + 9;
			break;
		default:
			break;
		}
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		if (n == 3 && question.person >= 0 && question.weapon >= 6 + 9 && question.location >= 6) {
			if (!making_accusation) {
				questions_asked++;
				change_clue_state(CLUE_TRANSMIT_QUESTION);
			} else {
				change_clue_state(CLUE_CHECK_ACCUSATION);
			}
		}
		screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		clue_run_idle_time = rtc_get_ms_since_boot();
		change_clue_state(CLUE_RUN);
	}
}

static void clue_received_answer(void)
{
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();

	new_evidence = -1;
	if (question_answer == -1) {
		FbMove(0, 10);
		FbWriteString(card[question_answer].name);
		FbMoveX(0);
		FbWriteString("\nKNOWS NOTHING\nABOUT THIS\n");
		update_screen();
	} else {
		for (int i = 0; i < NCARDS; i++) {
			if (current_deck.held_by[i] == question_answer &&
				(current_deck.card[i] == question.person ||
				current_deck.card[i] == question.location ||
				current_deck.card[i] == question.weapon)) {
				evidence.from_who[current_deck.card[i]] = question_answer;
				new_evidence = current_deck.card[i];
				change_clue_state(CLUE_EVIDENCE);
				return;
			}
		}
		FbMove(0, 10);
		FbWriteString(card[question_answer].name);
		FbMoveX(0);
		FbWriteString("\nKNOWS NOTHING\nABOUT THIS\n");
		update_screen();
	}
	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		clue_run_idle_time = rtc_get_ms_since_boot();
		change_clue_state(CLUE_RUN);
	}
}

static void clue_confirm_accuse(void)
{
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();

	FbMove(8, 8);
	FbWriteString("YOU ARE ABOUT\nTO MAKE A\nSERIOUS\nACCUSATION.\n\n");
	FbWriteString("IF YOU ARE\nINCORRECT\nYOU WILL LOSE\nTHE GAME.\n\n");
	FbWriteString("ARE YOU SURE\nYOU WANT TO\nPROCEED?\n");
	FbWriteString("\nA: NO\nB: YES\n");
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		clue_run_idle_time = rtc_get_ms_since_boot();
		change_clue_state(CLUE_RUN);
		return;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		change_clue_state(CLUE_ACCUSE);
		return;
	}
}

static int got_lucky(void)
{
	int count = 0;

	/* Count up the gathered evidence.  If more than 3 things are
	 * unaccounted for, then they guessed, rather than knew.
	 */
	for (int i = 0; i < NCARDS; i++) {
		if (evidence.from_who[i] == 255)
			count++;
	}
	return count > 3;
}

static void clue_check_accusation(void)
{
	int count = 0;
	printf("clue_check_accusation\n");

	for (int i = 0; i < NCARDS; i++) {
		if ((current_deck.card[i] == question.person ||
			current_deck.card[i] == question.location ||
			current_deck.card[i] == question.weapon) &&
			current_deck.held_by[i] == 255)
			count++;
	}
	if (count == 3) {
		/* They got the correct answer. */
		final_time_ms_since_boot = rtc_get_ms_since_boot();
		if (got_lucky())
				change_clue_state(CLUE_LUCKY);
			else
				change_clue_state(CLUE_CLEVER);
			return;
	}
	/* They got the wrong answer */
	change_clue_state(CLUE_WRONG_ACCUSATION);
}

static void clue_end(int lucky)
{
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();
	FbMove(8, 8);
	FbWriteString("YOUR ACCUSATION\nIS CORRECT!\n");
	FbWriteString("YOU ARE VERY\n");
	if (lucky)
		FbWriteString("LUCKY.\n");
	else
		FbWriteString("CLEVER.\n");
	draw_elapsed_time();
	draw_question_count();
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		game_in_progress = 0;
		change_clue_state(CLUE_INIT);
	}
}

static void clue_lucky(void)
{
	clue_end(1);
}

static void clue_clever(void)
{
	clue_end(0);
}

static void print_murder_thing(enum card_type t)
{
	for (int i = 0; i < NCARDS; i++) {
		if (current_deck.held_by[i] == 255) {
			if (card[current_deck.card[i]].type == t) {
				FbWriteString(card[current_deck.card[i]].name);
				if (t == CARD_TYPE_WEAPON) {
					FbWriteString("!\n");
					FbMoveX(8);
				} else {
					FbWriteString("\n");
					FbMoveX(8);
				}
				return;
			}
		}
	}
}

static void print_murderer(void)
{
	print_murder_thing(CARD_TYPE_SUSPECT);
}

static void print_murder_location(void)
{
	print_murder_thing(CARD_TYPE_ROOM);
}

static void print_murder_weapon(void)
{
	print_murder_thing(CARD_TYPE_WEAPON);
}

static void clue_wrong_accusation(void)
{
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();
	FbMove(8, 8);
	FbWriteString("YOUR ACCUSATION\nIS INCORRECT!\n\n");
	FbWriteString("IT WAS\n");
	print_murderer();
	FbWriteString("IN THE\n");
	print_murder_location();
	FbWriteString("WITH THE\n");
	print_murder_weapon();
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		game_in_progress = 0;
		change_clue_state(CLUE_INIT);
	}
}

static void init_evidence(void)
{
	/* We start with all the evidence from the cards dealt to us and nothing else. */
	for (int i = 0; i < NCARDS; i++) {
		if (current_deck.held_by[i] == playing_as_character)
			evidence.from_who[current_deck.card[i]] = playing_as_character;
		else
			evidence.from_who[current_deck.card[i]] = 255;
	}
}

static void clue_new_game(void)
{
	game_in_progress = 1;
	init_random_state();
	memset(&notebook, '?', sizeof(notebook));
	deal_cards(random_num_state, &current_deck);
	init_evidence();
	clue_run_idle_time = rtc_get_ms_since_boot();
	change_clue_state(CLUE_RUN);
	questions_asked = 0;
	start_time_ms_since_boot = rtc_get_ms_since_boot();
	final_time_ms_since_boot = 0;
}

static void build_and_send_packet(unsigned char address, unsigned short badge_id, uint64_t payload)
{
    IR_DATA ir_packet = {
            .data_length = 8,
            .recipient_address = badge_id,
            .app_address = address,
            .data = (uint8_t *) &payload,
    };
    ir_send_complete_message(&ir_packet);
}

static void clue_transmit_answer(void)
{
	static int counter = 0;
	uint64_t payload;

	payload = CLUESOE | (uint64_t) playing_as_character;
	counter++;
	if ((counter % 10) == 0) { /* transmit IR packet */
		build_and_send_packet(BADGE_IR_CLUE_GAME_ADDRESS, BADGE_IR_BROADCAST_ID, payload);
		audio_out_beep(500, 100);
	}
	FbClear();
	FbMove(10, 60);
	FbWriteLine("TRANSMITTING\n");
	FbWriteLine("ANSWER");
	update_screen();

	clue_run_idle_time = rtc_get_ms_since_boot();
	change_clue_state(CLUE_RUN);
}

static void clue_transmit_question(void)
{
	uint64_t question_packet;
	static int counter = 0;

	memcpy(&question_packet, "CLUE????", 8);

	counter++;
	if ((counter % 10) == 0) { /* transmit IR packet */
		build_and_send_packet(BADGE_IR_CLUE_GAME_ADDRESS, BADGE_IR_BROADCAST_ID, question_packet);
		audio_out_beep(500, 100);
	}
	FbClear();
	FbMove(10, 60);
	FbWriteLine("ASKING");
	FbMove(10, 70);
	FbWriteLine("QUESTION");
	update_screen();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		change_clue_state(CLUE_INTERVIEW);
	}
}

static uint64_t clue_get_payload(IR_DATA* packet)
{
	uint64_t data;
	memcpy(&data, packet->data, sizeof(data));
	return data;
}

static void clue_process_packet(IR_DATA* packet)
{
	uint64_t payload;

	payload = clue_get_payload(packet);
	udpdebug(stderr, "Got IR packet: %016lx\n", payload);
	if (memcmp(&payload, "CLUE????", 8) == 0) {
		change_clue_state(CLUE_TRANSMIT_ANSWER);
		return;
	}

	if ((payload & 0xffffffff00000000ul) == CLUESOE) {
		udpdebug(stderr, "Clue got answer payload\n");
		question_answer = (payload & 0x7);
		if (question_answer > NSUSPECTS - 1)
			question_answer = -1;
		udpdebug(stderr, "clue answer is suspect %d\n", question_answer);
		change_clue_state(CLUE_RECEIVED_ANSWER);
		return;
	}
	udpdebug(stderr, "Clue: got unexpected packet: 0x%016lx\n", payload);
}

static void clue_check_for_incoming_packets(void)
{
    IR_DATA *new_packet;
    IR_DATA new_packet_copy;
    uint8_t packet_data[64];
    int next_queue_out;
    uint32_t interrupt_state = hal_disable_interrupts();
    while (queue_out != queue_in) {
        next_queue_out = (queue_out + 1) % QUEUE_SIZE;
        new_packet = &packet_queue[queue_out];
        queue_out = next_queue_out;
	assert(new_packet->data_length <= 64);
	memcpy(&new_packet_copy, new_packet, sizeof(new_packet_copy));
	new_packet_copy.data = packet_data;
	memcpy(packet_data, new_packet->data, new_packet->data_length);
        hal_restore_interrupts(interrupt_state);
        clue_process_packet(&new_packet_copy);
        interrupt_state = hal_disable_interrupts();
    }
    hal_restore_interrupts(interrupt_state);
}

void clue_cb(void)
{
	if (scan_for_incoming_packets)
		clue_check_for_incoming_packets();
	switch (clue_state) {
	case CLUE_INIT:
		clue_init();
		break;
	case CLUE_INIT_MAIN_MENU:
		clue_init_main_menu();
		break;
	case CLUE_MAIN_MENU:
		clue_main_menu();
		break;
	case CLUE_NEW_GAME:
		clue_new_game();
		break;
	case CLUE_RUN:
		clue_run();
		break;
	case CLUE_NOTEBOOK:
		clue_notebook();
		break;
	case CLUE_EVIDENCE:
		clue_evidence();
		break;
	case CLUE_INTERVIEW:
		clue_interview(0);
		break;
	case CLUE_RECEIVED_ANSWER:
		clue_received_answer();
		break;
	case CLUE_CONFIRM_ACCUSE:
		clue_confirm_accuse();
		break;
	case CLUE_ACCUSE:
		clue_interview(1);
		break;
	case CLUE_CHECK_ACCUSATION:
		clue_check_accusation();
		break;
	case CLUE_TRANSMIT_QUESTION:
		clue_transmit_question();
		break;
	case CLUE_TRANSMIT_ANSWER:
		clue_transmit_answer();
		break;
	case CLUE_LUCKY:
		clue_lucky();
		break;
	case CLUE_CLEVER:
		clue_clever();
		break;
	case CLUE_WRONG_ACCUSATION:
		clue_wrong_accusation();
		break;
	case CLUE_EXIT:
		clue_exit();
		break;
	default:
		break;
	}
}

