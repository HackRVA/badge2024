#ifndef BADGE_H
#define BADGE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_APP_STACK_DEPTH 10

struct badge_app {
	void (*app_func)(struct badge_app *app);
	void *app_context;
	int wake_up;
};

void push_app(struct badge_app app);
void pop_app(void);

typedef struct {
    char name[16];
    uint64_t badgeId;
    char sekrits[8];
    char achievements[8];

    /*
       prefs
    */
    unsigned char ledBrightness;  /* 1 byte */
    unsigned char backlight;      /* 1 byte */
    bool mute;
    bool display_inverted;
    bool display_rotated;
    bool screensaver_inverted;
    bool screensaver_disabled;
} SYSTEM_DATA;

SYSTEM_DATA* badge_system_data(void);
void UserInit(void);
uint64_t ProcessIO(void);

#endif
