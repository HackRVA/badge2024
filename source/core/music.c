#include <stdio.h>

#include "music.h"
#include "audio.h"

static struct tune *current_tune;
static int current_note = 0;
static void (*finish_callback)(void) = NULL;
static int stop_the_music = 0;

static void next_note(void)
{
	if (!current_tune)
		return;
	current_note++;
	if (current_note >= current_tune->num_notes) {
		current_tune = NULL;
		if (finish_callback)
			finish_callback();
		return;
	}
	if (!stop_the_music) {
		audio_out_beep_with_cb(current_tune->note[current_note].freq,
			current_tune->note[current_note].duration, next_note);
	} else {
		current_tune = NULL;
		current_note = 0;
		stop_the_music = 0;
	}
}

void play_tune(struct tune *tune, void (*finished_callback)(void))
{
	stop_the_music = 0;
	current_note = 0;
	current_tune = tune;
	finish_callback = finished_callback;
	audio_out_beep_with_cb(current_tune->note[current_note].freq,
			current_tune->note[current_note].duration, next_note);
}

void stop_tune(void)
{
	stop_the_music = 1;
}

