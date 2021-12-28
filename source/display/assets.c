#include "assets.h"
#include "assetList.h"
#include "display_s6b33.h"

/**
    simple asset management and display lib
    Author: Paul Bruggeman
    paul@killercats.com
    4/2015
*/

/*
   255 = no asset active
*/
/** current global active asset */
unsigned char G_videoAssetId = 255;
/** current video frame */
unsigned int G_videoFrame = 0;

void drawAsset(unsigned char assetId)
{
    G_videoAssetId = assetId;
    assetList[assetId].datacb(assetId, G_videoFrame);
}

void drawLCD1(unsigned char assetId, int frame)
{
    unsigned char i, j, p, r, g, b, pixbyte;
    const unsigned char *cmap, *pixdata;
    unsigned short pixel ;

    // S6B33_rect(0, 0, assetList[assetId].x - 1, assetList[assetId].y - 1);
    S6B33_rect(0, 0, assetList[assetId].y - 1, assetList[assetId].x - 1);

    pixdata = &(assetList[assetId].pixdata[0]);
    for (i=0; i < assetList[assetId].y; i++) {
        for (j=0; j < assetList[assetId].x/8; j++ ) {
            pixbyte = *pixdata++; /* 8 pixels per byte */

            for (p=0; p<8; p++) {
                // 1st pixel
                //cmap = &(assetList[assetId].data_cmap[(unsigned short)((pixbyte>>(7-p)) & 0x1) * 3]);
                cmap = &(assetList[assetId].data_cmap[(unsigned short)((pixbyte>>p) & 0x1) * 3]);
                r = cmap[0];
                g = cmap[1];
                b = cmap[2];

                pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                          ( ((g >> 3) & 0b11111) <<  6 ) |
                          ( ((b >> 3) & 0b11111)       )) ;

                S6B33_pixel(pixel);
            }
        }
    }
}

void drawLCD2(unsigned char assetId, int frame)
{
    unsigned char i, j, r, g, b, pixbyte;
    const unsigned char *cmap, *pixdata;
    unsigned short pixel ;

    S6B33_rect(0, 0, assetList[assetId].x - 1, assetList[assetId].y - 1);

    pixdata = &(assetList[assetId].pixdata[0]);
    for (i=0; i < assetList[assetId].y; i++) {
        for (j=0; j < assetList[assetId].x/4; j++ ) { // 4 pixels at a ime
            pixbyte = *pixdata++; /* 2 pixels per byte */

            // 1st pixel
            cmap = &(assetList[assetId].data_cmap[(unsigned short)((pixbyte>>6) & 0x3) * 3]);
            r = cmap[0];
            g = cmap[1];
            b = cmap[2];

            pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                      ( ((g >> 3) & 0b11111) <<  6 ) |
                      ( ((b >> 3) & 0b11111)       )) ;

            S6B33_pixel(pixel);

            // 2nd pixel
            cmap = &(assetList[assetId].data_cmap[(unsigned short)((pixbyte>>4) & 0x3) * 3]);
            r = cmap[0];
            g = cmap[1];
            b = cmap[2];

            pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                      ( ((g >> 3) & 0b11111) <<  6 ) |
                      ( ((b >> 3) & 0b11111)       )) ;

            S6B33_pixel(pixel);

            // 3rd pixel
            cmap = &(assetList[assetId].data_cmap[(unsigned short)((pixbyte>>2) & 0x3) * 3]);
            r = cmap[0];
            g = cmap[1];
            b = cmap[2];

            pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                      ( ((g >> 3) & 0b11111) <<  6 ) |
                      ( ((b >> 3) & 0b11111)       )) ;

            S6B33_pixel(pixel);

            // 2nd pixel
            cmap = &(assetList[assetId].data_cmap[(unsigned short)(pixbyte & 0x3) * 3]);
            r = cmap[0];
            g = cmap[1];
            b = cmap[2];

            pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                      ( ((g >> 3) & 0b11111) <<  6 ) |
                      ( ((b >> 3) & 0b11111)       )) ;

            S6B33_pixel(pixel);
        }
    }
}

void drawLCD4(unsigned char assetId, int frame)
{
    unsigned char i, j, r, g, b, pixbyte;
    const unsigned char *cmap, *pixdata;
    unsigned short pixel ;

    S6B33_rect(0, 0, assetList[assetId].x - 1, assetList[assetId].y - 1);

    pixdata = &(assetList[assetId].pixdata[0]);
    for (i=0; i < assetList[assetId].y; i++) {
        for (j=0; j < assetList[assetId].x/2; j++ ) { // 2 pixels at a ime
            pixbyte = *pixdata++; /* 2 pixels per byte */

            // 1st pixel
            cmap = &(assetList[assetId].data_cmap[(unsigned short)(pixbyte>>4) * 3]);
            r = cmap[0];
            g = cmap[1];
            b = cmap[2];

            pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                      ( ((g >> 3) & 0b11111) <<  6 ) |
                      ( ((b >> 3) & 0b11111)       )) ;

            S6B33_pixel(pixel);

            // 2nd pixel
            cmap = &(assetList[assetId].data_cmap[(unsigned short)(pixbyte & 0xF) * 3]);
            r = cmap[0];
            g = cmap[1];
            b = cmap[2];

            pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                      ( ((g >> 3) & 0b11111) <<  6 ) |
                      ( ((b >> 3) & 0b11111)       )) ;

            S6B33_pixel(pixel);
        }
    }
}

void drawLCD8(unsigned char assetId, int frame)
{
    unsigned char i, j, r, g, b, pixbyte;
    const unsigned char *cmap;
    unsigned short pixel;

    S6B33_rect(0, 0, assetList[assetId].x - 1, assetList[assetId].y - 1);

    for (i=0; i < assetList[assetId].y; i++) {
        for (j=0; j < assetList[assetId].x; j++) {
            pixbyte = assetList[assetId].pixdata[i * assetList[assetId].x + j];
            cmap = &(assetList[assetId].data_cmap[(unsigned short)pixbyte * 3]);
            r = cmap[0];
            g = cmap[1];
            b = cmap[2];

            pixel = ( ( ((r >> 3) & 0b11111) << 11 ) |
                      ( ((g >> 3) & 0b11111) <<  6 ) |
                      ( ((b >> 3) & 0b11111)       )) ;

            S6B33_pixel(pixel);
        }
    }
}

// Sam: remove audio for now, move to another file
#if 0

#ifdef LOCKING_UP
volatile unsigned char G_audioAssetId = 255;
/* whether to even bother playing anything */
volatile char G_playing = 0;

/* number of samples for each audio step; a function of tempo */
volatile int G_samples_per_step = 0;

/* number of samples so far this step */
volatile int G_samples_cnt = 0;

/* how many notes we've gone in the note table */
volatile short G_note_num = 0;

/* total duration of note, in 1/16 note steps. 0 = hold */
volatile short G_duration = 0;

/* number of 1/16 note steps we've been playing this note */
volatile char G_duration_cnt = 0;

/* amount of the wave that will be skipped by each channel per cycle; a function of note frequency. 0 = silence */
volatile float G_wavehop[NUM_AUDIO_CHANNELS] = {0.0, 0.0};

/* duration through the wave that each channel has gone */
volatile float G_wavepos[NUM_AUDIO_CHANNELS] = {0.0, 0.0};

volatile unsigned char G_mute = 0;

void playAsset(unsigned char assetId)
{
    haltPlayback();
    G_audioAssetId = assetId;
    nextNote();
}

void haltPlayback(void)
{
    G_audioAssetId = 255;
    endNote();
    G_playing = 0;
    G_samples_cnt = 0;
    G_note_num = 0;
    G_duration_cnt = 0;
    int channel;
    for (channel = 0; channel < NUM_AUDIO_CHANNELS; channel++)
        G_wavepos[NUM_AUDIO_CHANNELS] = 0.0;
}

void setNote(unsigned short freq, unsigned short dur)
{
    G_wavehop[0] = (float) sizeof(wave_table) / (float) freq;

    G_samples_per_step = 4750;
    G_duration = dur / G_samples_per_step;
    G_playing = 1;

    int i;
    for (i = 1; i < NUM_AUDIO_CHANNELS; i++)
        G_wavehop[i] = 0;
}

void endNote(void)
{
    AUDIO_PHASE1 = 0;
    AUDIO_PHASE2 = 0;
    G_playing = 0;
}

// see include/audio.h
void doAudio(void)
{
    if (G_mute || !G_playing)
        return;

    signed char sample = 0;
    int channel;
    for (channel = 0; channel < NUM_AUDIO_CHANNELS; channel++) {

        if (!G_wavehop[channel])
            continue;

        sample += wave_table[(int) G_wavepos[channel]];
        G_wavepos[channel] += G_wavehop[channel];
        if ((int) G_wavepos[channel] >= (int) sizeof(wave_table)) {
            G_wavepos[channel] -= sizeof(wave_table);
        }
    }

    AUDIO_PHASE1 = (sample > 0);
    AUDIO_PHASE2 = (sample < 0);

    G_samples_cnt++;

    /* advance to the next step */
    if (G_samples_cnt >= G_samples_per_step) {
        G_samples_cnt = 0;
        G_duration_cnt++;

        /* advance to the next note */
        if (G_duration_cnt >= G_duration) {
            G_duration_cnt = 0;

            if (G_audioAssetId != 255)
                nextNote();
            else {
                endNote();
            }
        }
    }
}
#else

unsigned char G_audioAssetId = 255;
unsigned int G_audioFrame = 0; /* persistant current "frame" of audio, like a "frame" of video */
unsigned short G_currentNote=0;
unsigned short G_duration = 0;
unsigned short G_duration_cnt = 0;
unsigned short G_freq_cnt = 0;
unsigned short G_freq = 0;

unsigned char G_mute = 0;
unsigned short G_audio_phase = 0;
/* whether to even bother playing anything */
volatile char G_playing = 0;


/* how many notes we've gone in the note table */
volatile short G_note_num = 0;

/* number of samples for each audio step; a function of tempo */
volatile int G_samples_per_step = 0;

/* amount of the wave that will be skipped by each channel per cycle; a function of note frequency. 0 = silence */
//volatile float G_wavehop[NUM_AUDIO_CHANNELS] = {0.0, 0.0};

/* duration through the wave that each channel has gone */
//volatile float G_wavepos[NUM_AUDIO_CHANNELS] = {0.0, 0.0};


void playAsset(unsigned char assetId)
{
    G_audioAssetId = assetId;
    assetList[assetId].datacb(assetId, 0); /* install asset frame=0 */
}

void setNote(unsigned short freq, unsigned short dur) {
    if (G_mute) return;

    if (dur <= freq) dur = (freq << 1); /* to short to be play with PWM, so double it */

    G_freq = freq;
    G_duration = dur;

    G_freq_cnt = 0;
    G_duration_cnt = 0;
}

// RA9
void doAudio()
{
    if (G_duration == 0) return;

    G_freq_cnt++; /* current note freq counter */
    G_duration_cnt++;

    if (G_duration_cnt != G_duration) {
        if (G_freq_cnt == G_freq)  {
            G_freq_cnt = 0;
            switch (G_audio_phase) {
                case 0:
                    AUDIO_PHASE1 = 1;
                    AUDIO_PHASE2 = 0;
                    break;

                case 1:
                    AUDIO_PHASE1 = 0;
                    AUDIO_PHASE2 = 0;
                    break;

                case 2:
                    AUDIO_PHASE1 = 0;
                    AUDIO_PHASE2 = 1;
                    break;

                case 3:
                    AUDIO_PHASE1 = 0;
                    AUDIO_PHASE2 = 0;
                    break;
            }
            G_audio_phase++;
            G_audio_phase %= 4;
        }
        else {
            AUDIO_PHASE1 = 0;
            AUDIO_PHASE2 = 0;
            G_audio_phase = 0;
        }
    }
    else {
        G_duration = 0;

        AUDIO_PHASE1 = 0;
        AUDIO_PHASE2 = 0;
        G_audio_phase = 0;
        if (G_audioAssetId != 255) assetList[G_audioAssetId].datacb(G_audioAssetId, G_audioFrame) ; /* callback routine */
    }
    G_audioFrame++;
}

#endif


/* callback to feed next note to the audio function */
void nextNote(void)
{

#ifdef PEBXXX
    /* if the pattern is over */
    if (G_note_num >= assetList[G_audioAssetId].seqNum) {

        G_note_num = 0;

#ifdef BADGEIO
        /* kill sound if we're not looping */
        if (G_audioAssetId == BADGIO1) {
            G_audioAssetId = BADGIO2;
        } else if (G_audioAssetId == BADGIO2) {
            G_audioAssetId = BADGIO1;
        } else if (!assetList[G_audioAssetId].y) {
            endNote();
            G_audioAssetId = 255; /* clear curent asset */
            G_playing = 0;
            return;
        } else {
            G_note_num = assetList[G_audioAssetId].y - 1;
        }
#endif
    }

    G_playing = 1;

    char *line = &assetList[G_audioAssetId].pixdata[G_note_num * BYTES_PER_LINE];

    /* extract note information from asset */
    int channel;
    for (channel = 0; channel < NUM_AUDIO_CHANNELS; channel++)
        memcpy(&G_wavehop[channel], &line[channel * sizeof(float)], sizeof(float));

    G_duration = line[BYTES_PER_LINE - 1];
    G_note_num++;

    G_samples_per_step = assetList[G_audioAssetId].x;
#endif
}

#endif