//
// Created by Samuel Jones on 2/21/22.
// Implemented by Stephen M. Cameron Sun 07 May 2023 06:06:22 PM EDT
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef SIMULATOR_AUDIO
#include <portaudio.h>
#endif
#include <string.h>
#include <pthread.h>

#include "badge.h"
#include "audio.h"

#ifdef SIMULATOR_AUDIO
#define SAMPLE_RATE (48000)
/* 48 frames = 1 ms */
#define FRAMES_PER_BUFFER (48)
static int sound_device;
static int sound_working;
static PaStream *stream = NULL;
static void (*user_callback_fn)(void) = NULL;

/* 1 second worth of audio */
#define AUDIO_BUFFER_SIZE 48000
static float audio_buffer[AUDIO_BUFFER_SIZE] = { 0 };
static int audio_buffer_index = 0;
static int samples_left_to_play = 0;
static pthread_mutex_t audio_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

void audio_init_gpio(void)
{
    return;
}

#ifdef SIMULATOR_AUDIO
static void decode_paerror(PaError rc)
{
	if (rc == paNoError)
		return;
	fprintf(stderr, "An error occurred while using the portaudio stream\n");
	fprintf(stderr, "Error number: %d\n", rc);
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(rc));
}

static void terminate_portaudio(int rc)
{
	Pa_Terminate();
        decode_paerror(rc);
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int mixer_loop(__attribute__ ((unused)) const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	__attribute__ ((unused)) const PaStreamCallbackTimeInfo* timeInfo,
	__attribute__ ((unused)) PaStreamCallbackFlags statusFlags,
	__attribute__ ((unused)) void *userData )
{
	float *out = outputBuffer;

	pthread_mutex_lock(&audio_lock);
	if (samples_left_to_play == 0 && user_callback_fn) {
		void (*temp_callback_fn)(void) = user_callback_fn;
		user_callback_fn = NULL;
		pthread_mutex_unlock(&audio_lock);
		temp_callback_fn();
		pthread_mutex_lock(&audio_lock);
	}
	if (badge_system_data()->mute) {
		memset(out, 0, sizeof(*out) * framesPerBuffer);
	} else {
		for (size_t i = 0; i < framesPerBuffer; i++) {
			out[i] = audio_buffer[audio_buffer_index];
			audio_buffer_index++;
			if (audio_buffer_index >= AUDIO_BUFFER_SIZE)
				audio_buffer_index = 0;
		}
	}
	samples_left_to_play -= framesPerBuffer;
	if (samples_left_to_play <= 0) {
		memset(audio_buffer, 0, sizeof(audio_buffer));
		samples_left_to_play = 0;
	}
	pthread_mutex_unlock(&audio_lock);
	return 0; /* we're never finished */
}
#endif

void audio_init(void)
{
#ifdef SIMULATOR_AUDIO
	printf("Initializing portaudio..."); fflush(stdout);

	PaStreamParameters outparams;
	PaError rc;
	PaDeviceIndex device_count;

	rc = Pa_Initialize();
	if (rc != paNoError)
		goto error;

	device_count = Pa_GetDeviceCount();
	printf("Portaudio reports %d sound devices.\n", device_count);

	if (device_count == 0) {
		printf("There will be no audio.\n");
		goto error;
		rc = 0;
	}
	sound_working = 1;

	outparams.device = Pa_GetDefaultOutputDevice();  /* default output device */

	printf("Portaudio says the default device is: %d\n", outparams.device);
	printf("Using sound device %d\n", outparams.device);
	sound_device = outparams.device;

	if (outparams.device < 0 && device_count > 0) {
		printf("Hmm, that's strange, portaudio says the default device is %d,\n"
			" but there are %d devices\n",
			outparams.device, device_count);
		printf("I think we'll just skip sound for now.\n");
		sound_working = 0;
		return;
	}

	outparams.channelCount = 1;                      /* mono output */
	outparams.sampleFormat = paFloat32;              /* 32 bit floating point output */
	outparams.suggestedLatency =
		Pa_GetDeviceInfo(outparams.device)->defaultLowOutputLatency;
	outparams.hostApiSpecificStreamInfo = NULL;

	rc = Pa_OpenStream(&stream,
		NULL,         /* no input */
		&outparams, SAMPLE_RATE, FRAMES_PER_BUFFER,
		paNoFlag, /* paClipOff, */   /* we won't output out of range samples so don't bother clipping them */
		mixer_loop, NULL /* cookie */);
	if (rc != paNoError)
		goto error;
	if ((rc = Pa_StartStream(stream)) != paNoError)
		goto error;
	return;
error:
	terminate_portaudio(rc);
	return;
#endif
}

int audio_out_beep_with_cb(uint16_t freq,  uint16_t duration, void (*beep_finished)(void))
{
#ifdef SIMULATOR_AUDIO
	float value = -0.025;

	if (duration <= 0)
		return 0;

	if (freq == 0 && beep_finished != NULL) { /* We're being asked to play a rest? Ok. */
		pthread_mutex_lock(&audio_lock);
		memset(audio_buffer, 0, sizeof(audio_buffer));
		user_callback_fn = beep_finished;
		audio_buffer_index = 0;
		samples_left_to_play = (duration * 48);
		if (samples_left_to_play > AUDIO_BUFFER_SIZE)
			samples_left_to_play = AUDIO_BUFFER_SIZE;
		pthread_mutex_unlock(&audio_lock);
		return 0;
	}

	int count = AUDIO_BUFFER_SIZE / freq / 2;
	pthread_mutex_lock(&audio_lock);
	for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
		audio_buffer[i] = value;
		if ((i % count) == 0)
			value = -value;
	}
	user_callback_fn = beep_finished;
	audio_buffer_index = 0;
	samples_left_to_play = (duration * 48);
	if (samples_left_to_play > AUDIO_BUFFER_SIZE)
		samples_left_to_play = AUDIO_BUFFER_SIZE;
	pthread_mutex_unlock(&audio_lock);
#endif
	return 0;
}

int audio_out_beep(uint16_t freq, uint16_t duration)
{
	return audio_out_beep_with_cb(freq, duration, NULL);
}

void audio_stby_ctl( __attribute__((__unused__)) bool enabled)
{
    return;
}
