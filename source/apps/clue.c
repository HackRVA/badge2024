#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

#define BADGE_IR_CLUE_GAME_ADDRESS IR_APP3
#define BADGE_IR_BROADCAST_ID 0
#define CLUE_QUESTION_OPCODE 0x01
#define CLUE_ANSWER_OPCODE 0x03

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

static int game_in_progress = 0;
static signed char playing_as_character = -1;
static int questions_asked = 0;
static unsigned int clue_random_seed = 0;



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
	CLUE_ANSWER,
	CLUE_ACCUSE,
	CLUE_TRANSMIT_QUESTION,
	CLUE_TRANSMIT_ANSWER,
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
static struct deck scratch_deck = { 0 };

static unsigned int random_num_state = 0;
static unsigned int question_random_seed = 0;
static int question_answer = -1;
static int new_evidence = -1;
static struct question remote_question = { 0 };

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
	murder_location = random_num(NLOCATIONS);
	murder_weapon = random_num(NWEAPONS);

	d->held_by[murderer] = 255;
	d->held_by[murder_location] = 255;
	d->held_by[murder_weapon] = 255;

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
static int screen_changed = 0;

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
	clue_state = CLUE_INIT_MAIN_MENU;
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
	dynmenu_add_item(&main_menu, "PAUSE_GAME", CLUE_EXIT, 2);

	dynmenu_init(&game_menu, game_menu_item, ARRAYSIZE(game_menu_item));
	dynmenu_clear(&game_menu);
	strcpy(game_menu.title, "CLUE");
	strcpy(game_menu.title2, "MY NAME IS:");
	strcpy(game_menu.title3, card[playing_as_character].name);
	dynmenu_add_item(&game_menu, "NOTEBOOK", CLUE_NOTEBOOK, 0);
	dynmenu_add_item(&game_menu, "EVIDENCE", CLUE_EVIDENCE, 1);
	dynmenu_add_item(&game_menu, "QSTN SUSPECT", CLUE_INTERVIEW, 2);
	dynmenu_add_item(&game_menu, "ANSWER QSTN", CLUE_ANSWER, 2);
	dynmenu_add_item(&game_menu, "ACCUSE", CLUE_ACCUSE, 3);
	dynmenu_add_item(&game_menu, "PAUSE GAME", CLUE_EXIT, 4);

	clue_state = CLUE_MAIN_MENU;
}

static void clue_main_menu(void)
{
	dynmenu_draw(&main_menu);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		dynmenu_change_current_selection(&main_menu, 1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		dynmenu_change_current_selection(&main_menu, -1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		clue_state = main_menu.item[main_menu.current_item].next_state;
	}
}

static void clue_run()
{
	FbClear();
	dynmenu_draw(&game_menu);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		dynmenu_change_current_selection(&game_menu, 1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		dynmenu_change_current_selection(&game_menu, -1);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		clue_state = game_menu.item[game_menu.current_item].next_state;
	}
}

static void clue_exit()
{
	clue_state = CLUE_INIT; /* So that when we start again, we do not immediately exit */
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
	FbSwapBuffers();

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
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		row--;
		if (row < 0)
			row = 0;
		if (row < top_item)
			top_item = row;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		col++;
		if (col > 5)
			col = 5;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		col--;
		if (col < 0)
			col = 0;
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
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		clue_state = CLUE_RUN;
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

	if (new_evidence >= 0) {
		int nc = 0;
		for (int i = 0; i < NCARDS; i++) {
			if (evidence.from_who[i] != 255) {
				nc++;
				if (new_evidence == i) {
					n = nc - 1;
					ni = i;
					new_evidence = -1;
					break;
				}
			}
		}
	}

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
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		n++;
		if (n > count - 1)
			n = count - 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		n--;
		if (n < 0)
			n = 0;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		clue_state = CLUE_RUN;
	}
}

static void clue_interview(void)
{
	static int n = 0;
	FbClear();

	FbMove(1, 1);
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbWriteString("QUESTION:\n\n");
	FbWriteString("I THINK\n");
	FbColor(n == 0 ? GREEN : YELLOW);
	if (question.person == -1) {
		FbWriteString("PERSON\n");
	} else {
		FbWriteString(card[question.person].name);
		FbMoveX(0);
		FbWriteString("\n");
	}
	FbColor(WHITE);
	FbWriteString("\nDID IT IN THE\n");
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
	FbWriteString("\n\nASK QUESTION");
	FbColor(WHITE);
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		n++;
		if (n > 3)
			n = 3;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		n--;
		if (n < 0)
			n = 0;
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
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		if (n == 3 && question.person >= 0 && question.weapon >= 6 + 9 && question.location >= 6) {
			clue_state = CLUE_TRANSMIT_QUESTION;
		}
	}
}

static void clue_answer(void)
{
	/* Replicate the remote deck */
	deal_cards(question_random_seed, &scratch_deck);
	int found;

	/* scan the deck to see if we have any cards in the question */
	found = -1;
	for (int i = 0; i < NCARDS; i++) {
		if (scratch_deck.card[i] == question.person ||
			scratch_deck.card[i] == question.location ||
			scratch_deck.card[i] == question.weapon) {
			if (scratch_deck.held_by[i] == playing_as_character) {
				found = i;
				break;
			}
		}
	}
	question_answer = found;
	clue_state = CLUE_TRANSMIT_ANSWER;
}

static void clue_accuse(void)
{
}

static void init_evidence(void)
{
	/* We start with all the evidence from the cards dealt to us and nothing else. */
	for (int i = 0; i < NCARDS; i++) {
		if (current_deck.held_by[i] == playing_as_character)
			evidence.from_who[i] = playing_as_character;
		else
			evidence.from_who[i] = 255;
	}
}

static void clue_new_game(void)
{
	game_in_progress = 1;
	init_random_state();
	memset(&notebook, '?', sizeof(notebook));
	deal_cards(random_num_state, &current_deck);
	init_evidence();
	clue_state = CLUE_RUN;
	questions_asked = 0;
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

	payload = (uint64_t) question_random_seed << 32;
	payload |= ((CLUE_ANSWER_OPCODE & 0x3) << 10);
	payload |= ((playing_as_character & 0x7) << 5);
	payload |= (question_answer & 0x1f);

	counter++;
	if ((counter % 10) == 0) { /* transmit IR packet */
		build_and_send_packet(BADGE_IR_CLUE_GAME_ADDRESS, BADGE_IR_BROADCAST_ID, payload);
	    audio_out_beep(500, 100);
	}
	FbClear();
	FbMove(10, 60);
	FbWriteLine("TRANSMITTING\n");
	FbWriteLine("ANSWER");
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		clue_state = CLUE_RUN;
	}
}

static void clue_transmit_question(void)
{
	uint64_t question_packet;
	static int counter = 0;


	question_packet = (uint64_t) clue_random_seed << 32;		/* 32 bits */
	question_packet |= (question.person & 0x7);			/* 3 bits */
	question_packet |= ((question.location & 0x0f) << 3);		/* 4 bits */
	question_packet |= ((question.weapon & 0x07) << 7);		/* 3 bits */
	question_packet |= ((CLUE_QUESTION_OPCODE & 0x3) << 10);	/* 2 bits */

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
	FbSwapBuffers();

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		clue_state = CLUE_INTERVIEW;
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
	unsigned char opcode;

	payload = clue_get_payload(packet);
	opcode = (payload >> 10) & 0x03;

	printf("Got IR packet: %016lx\n", payload);

	if (opcode == CLUE_QUESTION_OPCODE) {
		question_random_seed = (unsigned int) ((payload >> 32) & 0xffffffffllu);
		remote_question.person = (payload & 0x7);
		remote_question.location = (payload >> 3) & 0x0f;
		remote_question.weapon = (payload >> 7) & 0x7;
#if TARGET_SIMULATOR
		if (remote_question.person < 0 || remote_question.person > 5)
			printf("Bad question received, person = %d\n", remote_question.person);
		if (remote_question.location < 6 || remote_question.location > 6 + 9 - 1)
			printf("Bad question received, location = %d\n", remote_question.location);
		if (remote_question.weapon < 6 + 9 || remote_question.weapon > 20)
			printf("Bad question received, weapon = %d\n", remote_question.weapon);
#endif
		clue_state = CLUE_ANSWER;
	}

	if (opcode == CLUE_ANSWER_OPCODE) {
		int from_who;
		int which_card;
		unsigned int rs = (unsigned int) ((payload >> 32) & 0xffffffffllu);
		if (rs != clue_random_seed) {
			printf("Got answer, but seed value is incorrect.\n");
		}
		which_card = payload & 0x1f;
		if (which_card == 0x1f) {
			printf("Suspect doesn't know anything.\n");
		} else {
			from_who = (payload >> 5) & 0x7;
			if (from_who < 0 || from_who > 5) {
				printf("Bad answer, suspect is out of range\n");
			} else {
				new_evidence = which_card;
				evidence.from_who[new_evidence] = from_who;
				clue_state = CLUE_EVIDENCE;
			}
		}
	}
}

static void clue_check_for_incoming_packets(void)
{
    IR_DATA *new_packet;
    int next_queue_out;
    uint32_t interrupt_state = hal_disable_interrupts();
    while (queue_out != queue_in) {
        next_queue_out = (queue_out + 1) % QUEUE_SIZE;
        new_packet = &packet_queue[queue_out];
        queue_out = next_queue_out;
        hal_restore_interrupts(interrupt_state);
        clue_process_packet(new_packet);
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
		clue_interview();
		break;
	case CLUE_ANSWER:
		clue_answer();
		break;
	case CLUE_ACCUSE:
		clue_accuse();
		break;
	case CLUE_TRANSMIT_QUESTION:
		clue_transmit_question();
		break;
	case CLUE_TRANSMIT_ANSWER:
		clue_transmit_answer();
		break;
	case CLUE_EXIT:
		clue_exit();
		break;
	default:
		break;
	}
}

