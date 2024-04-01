#ifndef MUSIC_H__
#define MUSIC_H__
#include <stdint.h>
#include <stdbool.h>

#define NOTE_REST 0
#define NOTE_A2 110
#define NOTE_As2 117
#define NOTE_Bf2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_Cs3 139
#define NOTE_Df3 139
#define NOTE_D3 147
#define NOTE_Ds3 156
#define NOTE_Ef3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_Fs3 185
#define NOTE_Gf3 185
#define NOTE_G3 196
#define NOTE_Gs3 208
#define NOTE_Af3 208
#define NOTE_A3 220
#define NOTE_As3 233
#define NOTE_Bf3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_Cs4 277
#define NOTE_Df4 277
#define NOTE_D4 294
#define NOTE_Ds4 311
#define NOTE_Ef4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_Fs4 370
#define NOTE_Gf4 370
#define NOTE_G4 392
#define NOTE_Gs4 415
#define NOTE_Af4 415
#define NOTE_A4 440
#define NOTE_As4 466
#define NOTE_Bf4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_Cs5 554
#define NOTE_Df5 554
#define NOTE_D5 587
#define NOTE_Ds5 622
#define NOTE_Ef5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_Fs5 740
#define NOTE_Gf5 740
#define NOTE_G5 784
#define NOTE_Gs5 831
#define NOTE_Af5 831
#define NOTE_A5 880
#define NOTE_As5 932
#define NOTE_Bf5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_Cs6 1109
#define NOTE_Df6 1109
#define NOTE_D6 1175
#define NOTE_Ds6 1245
#define NOTE_Ef6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_Fs6 1480
#define NOTE_Gf6 1480
#define NOTE_G6 1568
#define NOTE_Gs6 1661
#define NOTE_Af6 1661
#define NOTE_A6 1760
#define NOTE_As6 1865
#define NOTE_Bf6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_Cs7 2217
#define NOTE_Df7 2217
#define NOTE_D7 2349
#define NOTE_Ds7 2489
#define NOTE_Ef7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_Fs7 2960
#define NOTE_Gf7 2960
#define NOTE_G7 3322
#define NOTE_Gs7 3729
#define NOTE_Af7 3729
#define NOTE_A7 4435
#define NOTE_As7 4699
#define NOTE_Bf7 4699
#define NOTE_B7 4978
#define NOTE_C8 5920
#define NOTE_Cs8 6645
#define NOTE_Df8 6645
#define NOTE_D8 7459
#define NOTE_Ds8 7902
#define NOTE_Ef8 7902

struct note {
	uint16_t freq, duration; // 0 frequency means rest
};

struct tune {
	int num_notes;
	struct note *note;
};

void play_tune(struct tune *tune, void (*finished_callback)(void *cookie), void *cookie);

void stop_tune(void);

#endif
