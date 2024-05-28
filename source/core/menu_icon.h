#ifndef MENU_ICON_H__
#define MENU_ICON_H__

#include "framebuffer.h"

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

struct menu_icon {
	int npoints;
	int color;
	const struct point *points;
};

const struct point games_icon_points[] = {
	{ -60, 28 },
	{ -45, -38 },
	{ -35, -44 },
	{ -22, -36 },
	{ 22, -37 },
	{ 33, -48 },
	{ 45, -34 },
	{ 59, 27 },
	{ 49, 33 },
	{ 28, 24 },
	{ 23, 13 },
	{ -20, 13 },
	{ -25, 24 },
	{ -52, 37 },
	{ -60, 28 },
	{ -128, -128 },
	{ -30, -15 },
	{ -30, -25 },
	{ -25, -24 },
	{ -24, -16 },
	{ -16, -15 },
	{ -15, -10 },
	{ -25, -10 },
	{ -26, -2 },
	{ -32, -11 },
	{ -39, -14 },
	{ -39, -18 },
	{ -30, -17 },
	{ -128, -128 },
	{ 9, -15 },
	{ 9, -9 },
	{ 15, -9 },
	{ 14, -18 },
	{ 10, -16 },
	{ -128, -128 },
	{ 23, -24 },
	{ 29, -25 },
	{ 29, -17 },
	{ 22, -18 },
	{ 23, -23 },
	{ -128, -128 },
	{ 34, -14 },
	{ 40, -14 },
	{ 38, -6 },
	{ 33, -7 },
	{ 34, -15 },
	{ -128, -128 },
	{ 20, -5 },
	{ 21, 2 },
	{ 27, 1 },
	{ 27, -4 },
	{ 20, -5 },
	{ -128, -128 },
};

const struct point settings_icon_points[] = {
	{ -7, -58 },
	{ 11, -58 },
	{ 11, -46 },
	{ 25, -39 },
	{ 35, -46 },
	{ 49, -37 },
	{ 41, -22 },
	{ 46, -11 },
	{ 60, -7 },
	{ 61, 9 },
	{ 45, 12 },
	{ 41, 25 },
	{ 47, 37 },
	{ 34, 49 },
	{ 26, 42 },
	{ 13, 46 },
	{ 9, 60 },
	{ -8, 61 },
	{ -11, 49 },
	{ -22, 42 },
	{ -34, 49 },
	{ -47, 38 },
	{ -39, 24 },
	{ -44, 14 },
	{ -59, 10 },
	{ -59, -9 },
	{ -43, -12 },
	{ -38, -23 },
	{ -47, -36 },
	{ -35, -46 },
	{ -23, -39 },
	{ -12, -44 },
	{ -8, -58 },
	{ -128, -128 },
	{ 0, -24 },
	{ 13, -19 },
	{ 22, -10 },
	{ 25, -1 },
	{ 22, 10 },
	{ 14, 21 },
	{ 1, 24 },
	{ -10, 21 },
	{ -20, 11 },
	{ -23, -1 },
	{ -20, -12 },
	{ -10, -20 },
	{ 1, -24 },
};

const struct point schedule_icon_points[] = {
	{ -55, -40 },
	{ -48, -48 },
	{ 30, -47 },
	{ 39, -38 },
	{ 37, 13 },
	{ 24, 16 },
	{ 18, 21 },
	{ 14, 28 },
	{ 13, 42 },
	{ 16, 45 },
	{ -47, 45 },
	{ -55, 38 },
	{ -54, -39 },
	{ -128, -128 },
	{ -52, -19 },
	{ 38, -20 },
	{ -128, -128 },
	{ -54, 2 },
	{ 37, 3 },
	{ -128, -128 },
	{ -53, 23 },
	{ 14, 25 },
	{ -128, -128 },
	{ -33, -18 },
	{ -33, 44 },
	{ -128, -128 },
	{ -9, -19 },
	{ -10, 43 },
	{ -128, -128 },
	{ 18, -18 },
	{ 17, 20 },
	{ -128, -128 },
	{ 38, 14 },
	{ 50, 19 },
	{ 57, 29 },
	{ 56, 40 },
	{ 51, 51 },
	{ 40, 55 },
	{ 27, 56 },
	{ 19, 51 },
	{ 17, 46 },
	{ -128, -128 },
	{ 35, 22 },
	{ 36, 39 },
	{ 45, 44 },
	{ -128, -128 },
	{ -33, -57 },
	{ -34, -42 },
	{ -128, -128 },
	{ 16, -56 },
	{ 16, -40 },
};

const struct point about_icon_points[] = {
	{ -53, -1 },
	{ -53, -26 },
	{ -47, -40 },
	{ -36, -50 },
	{ -19, -53 },
	{ 3, -55 },
	{ 20, -53 },
	{ 39, -48 },
	{ 50, -34 },
	{ 55, -15 },
	{ 55, 8 },
	{ 50, 23 },
	{ 39, 37 },
	{ 25, 43 },
	{ 10, 47 },
	{ -8, 46 },
	{ -20, 42 },
	{ -30, 36 },
	{ -37, 47 },
	{ -59, 57 },
	{ -45, 42 },
	{ -41, 27 },
	{ -51, 20 },
	{ -54, -1 },
	{ -128, -128 },
	{ -18, 27 },
	{ 16, 27 },
	{ 5, 17 },
	{ 4, -27 },
	{ -17, -14 },
	{ -8, -9 },
	{ -8, 17 },
	{ -18, 27 },
	{ -128, -128 },
	{ -15, -38 },
	{ -5, -46 },
	{ 4, -38 },
	{ -5, -31 },
	{ -15, -38 },
};

const struct point battlezone_icon_points[] = {
	{ -39, 15 },
	{ 42, 14 },
	{ 53, -4 },
	{ 48, -8 },
	{ -48, -9 },
	{ -54, -2 },
	{ -38, 13 },
	{ -128, -128 },
	{ -20, -14 },
	{ -22, -17 },
	{ -15, -29 },
	{ 13, -25 },
	{ 22, -15 },
	{ 16, -12 },
	{ -128, -128 },
	{ 19, -19 },
	{ 61, -19 },
	{ 60, -17 },
	{ 19, -16 },
	{ -128, -128 },
	{ -8, -28 },
	{ -14, -37 },
	{ -16, -46 },
	{ -9, -32 },
	{ 2, -30 },
	{ 2, -28 },
	{ -128, -128 },
};

const struct point lunar_rescue_icon_points[] = {
	{ -64, 20 },
	{ -47, 23 },
	{ -32, 23 },
	{ -10, 18 },
	{ 7, 15 },
	{ 24, 8 },
	{ 48, 14 },
	{ 55, 20 },
	{ 62, 22 },
	{ -128, -128 },
	{ 9, 36 },
	{ 12, 33 },
	{ 24, 32 },
	{ 55, 32 },
	{ 61, 35 },
	{ 55, 41 },
	{ 37, 42 },
	{ 18, 41 },
	{ 12, 39 },
	{ 9, 38 },
	{ -1, 42 },
	{ -23, 49 },
	{ -128, -128 },
	{ 60, 35 },
	{ 63, 38 },
	{ -128, -128 },
	{ -55, 31 },
	{ -49, 29 },
	{ -40, 31 },
	{ -47, 34 },
	{ -54, 30 },
	{ -61, 33 },
	{ -128, -128 },
	{ -41, 31 },
	{ -35, 36 },
	{ -128, -128 },
	{ -19, -29 },
	{ -21, -22 },
	{ -19, -14 },
	{ -11, -15 },
	{ -9, -21 },
	{ -12, -28 },
	{ -18, -29 },
	{ -128, -128 },
	{ -10, -18 },
	{ -5, -14 },
	{ -5, -8 },
	{ -128, -128 },
	{ -21, -20 },
	{ -26, -14 },
	{ -27, -8 },
	{ -128, -128 },
	{ -16, -24 },
	{ -17, -21 },
	{ -14, -21 },
	{ -15, -23 },
	{ -128, -128 },
	{ -18, -13 },
	{ -22, 0 },
	{ -18, -10 },
	{ -20, 9 },
	{ -17, 5 },
	{ -17, 20 },
	{ -15, 17 },
	{ -16, 30 },
	{ -14, 16 },
	{ -11, 17 },
	{ -12, 6 },
	{ -8, 10 },
	{ -12, -5 },
	{ -7, -2 },
	{ -14, -13 },
};

const struct point legacy_games_icon_points[] = {
	{ -52, 26 },
	{ -52, 50 },
	{ 51, 51 },
	{ 48, 27 },
	{ -51, 25 },
	{ -33, 9 },
	{ -12, 7 },
	{ -19, 15 },
	{ -17, 19 },
	{ 24, 19 },
	{ 30, 14 },
	{ 14, 0 },
	{ 13, -37 },
	{ 16, -44 },
	{ 15, -55 },
	{ 3, -59 },
	{ -15, -55 },
	{ -15, -41 },
	{ -11, -37 },
	{ -8, -32 },
	{ -8, -1 },
	{ -13, 6 },
	{ -128, -128 },
	{ 24, 5 },
	{ 32, 6 },
	{ 47, 24 },
	{ -128, -128 },
	{ -44, 19 },
	{ -40, 22 },
	{ -31, 22 },
	{ -26, 19 },
	{ -35, 17 },
	{ -39, 18 },
	{ -44, 20 },
	{ -43, 24 },
	{ -34, 28 },
	{ -28, 26 },
	{ -26, 21 },
};

const struct point badge_monsters_icon_points[] = {
	{ -48, -47 },
	{ -41, -58 },
	{ 40, -58 },
	{ 46, -49 },
	{ 45, 53 },
	{ 39, 60 },
	{ -47, 61 },
	{ -50, 55 },
	{ -48, -48 },
	{ -128, -128 },
	{ -3, -54 },
	{ -11, -50 },
	{ -16, -37 },
	{ -26, -32 },
	{ -39, -28 },
	{ -46, -15 },
	{ -46, 5 },
	{ -42, 14 },
	{ -33, 13 },
	{ -29, 5 },
	{ -37, -4 },
	{ -35, -13 },
	{ -20, -18 },
	{ -15, -1 },
	{ -24, 15 },
	{ -27, 40 },
	{ -37, 49 },
	{ -41, 56 },
	{ -23, 53 },
	{ -22, 42 },
	{ -15, 21 },
	{ -3, 13 },
	{ 8, 26 },
	{ 13, 45 },
	{ 13, 50 },
	{ 27, 54 },
	{ 25, 47 },
	{ 17, 42 },
	{ 17, 25 },
	{ 4, 0 },
	{ 12, -17 },
	{ 21, -8 },
	{ 24, 12 },
	{ 18, 19 },
	{ 23, 27 },
	{ 32, 19 },
	{ 32, -13 },
	{ 10, -35 },
	{ 7, -51 },
	{ -3, -54 },
	{ -128, -128 },
	{ -9, -43 },
	{ -5, -41 },
	{ -128, -128 },
	{ 0, -40 },
	{ 4, -42 },
	{ -128, -128 },
	{ -11, -32 },
	{ -7, -35 },
	{ 2, -35 },
	{ 7, -27 },
	{ -128, -128 },
	{ -9, -34 },
	{ -8, -30 },
	{ -6, -35 },
	{ -128, -128 },
	{ -1, -34 },
	{ 0, -28 },
	{ 1, -34 },
};

const struct point backlight_icon_points[] = {
	{ -1, -38 },
	{ -15, -37 },
	{ -25, -28 },
	{ -29, -15 },
	{ -28, -5 },
	{ -22, 2 },
	{ -15, 11 },
	{ -9, 23 },
	{ -8, 33 },
	{ 11, 32 },
	{ 13, 19 },
	{ 18, 4 },
	{ 22, -2 },
	{ 24, -12 },
	{ 19, -27 },
	{ 14, -35 },
	{ -2, -39 },
	{ -128, -128 },
	{ -6, 15 },
	{ -10, -18 },
	{ -6, -15 },
	{ -3, -18 },
	{ 1, -13 },
	{ 6, -18 },
	{ 7, -13 },
	{ 12, -17 },
	{ 7, 17 },
	{ -128, -128 },
	{ -36, 22 },
	{ -26, 16 },
	{ -128, -128 },
	{ -49, -9 },
	{ -38, -10 },
	{ -128, -128 },
	{ -45, -42 },
	{ -35, -34 },
	{ -128, -128 },
	{ -17, -57 },
	{ -15, -47 },
	{ -128, -128 },
	{ 21, -57 },
	{ 16, -46 },
	{ -128, -128 },
	{ 42, -41 },
	{ 28, -29 },
	{ -128, -128 },
	{ 50, -18 },
	{ 31, -17 },
	{ -128, -128 },
	{ 30, 10 },
	{ 43, 18 },
	{ -128, -128 },
	{ -7, 38 },
	{ 9, 38 },
	{ -128, -128 },
	{ -7, 45 },
	{ 8, 45 },
	{ -128, -128 },
	{ -5, 51 },
	{ 7, 51 },
};

const struct point led_icon_points[] = {
	{ -32, 19 },
	{ -33, 29 },
	{ 35, 29 },
	{ 35, 19 },
	{ -32, 16 },
	{ -128, -128 },
	{ -25, 14 },
	{ -26, -40 },
	{ -24, -49 },
	{ -16, -60 },
	{ 0, -63 },
	{ 20, -59 },
	{ 26, -52 },
	{ 31, -43 },
	{ 31, -24 },
	{ 29, 17 },
	{ -128, -128 },
	{ -19, -21 },
	{ -19, -46 },
	{ -16, -52 },
	{ -6, -57 },
	{ -128, -128 },
	{ -19, 29 },
	{ -18, 53 },
	{ -16, 52 },
	{ -15, 29 },
	{ -128, -128 },
	{ 17, 29 },
	{ 17, 59 },
	{ 19, 57 },
	{ 20, 28 },
	{ -128, -128 },
	{ -51, -8 },
	{ -33, -7 },
	{ -128, -128 },
	{ -50, -44 },
	{ -34, -34 },
	{ -128, -128 },
	{ -37, -62 },
	{ -28, -53 },
	{ -128, -128 },
	{ 38, -54 },
	{ 46, -61 },
	{ -128, -128 },
	{ 41, -36 },
	{ 59, -44 },
	{ -128, -128 },
	{ 42, -10 },
	{ 55, -9 },
};

const struct point audio_icon_points[] = {
	{ -38, -27 },
	{ -38, 19 },
	{ -25, 18 },
	{ -25, -25 },
	{ -38, -28 },
	{ -128, -128 },
	{ -20, -30 },
	{ 15, -62 },
	{ 9, -51 },
	{ 4, -31 },
	{ 0, -4 },
	{ 3, 23 },
	{ 5, 46 },
	{ 13, 61 },
	{ 22, 45 },
	{ 25, 15 },
	{ 24, -5 },
	{ 25, -25 },
	{ 21, -54 },
	{ 15, -63 },
	{ -128, -128 },
	{ -25, 17 },
	{ 12, 60 },
	{ -128, -128 },
	{ 3, 13 },
	{ 7, 5 },
	{ 6, -7 },
	{ 3, -17 },
	{ -128, -128 },
	{ -20, -29 },
	{ -25, -24 },
};

const struct point clear_nvram_icon_points[] = {
	{ 24, 30 },
	{ 24, 45 },
	{ 53, 26 },
	{ 53, 9 },
	{ 24, 29 },
	{ -56, -17 },
	{ -26, -28 },
	{ 52, 9 },
	{ -128, -128 },
	{ -57, -15 },
	{ -55, 2 },
	{ 23, 43 },
	{ -128, -128 },
	{ 17, 33 },
	{ 11, 42 },
	{ 11, 54 },
	{ -128, -128 },
	{ 7, 28 },
	{ 1, 35 },
	{ 1, 47 },
	{ -128, -128 },
	{ -6, 24 },
	{ -12, 29 },
	{ -12, 44 },
	{ -128, -128 },
	{ -15, 17 },
	{ -21, 23 },
	{ -22, 35 },
	{ -128, -128 },
	{ -25, 10 },
	{ -33, 16 },
	{ -30, 30 },
	{ -128, -128 },
	{ -35, 3 },
	{ -42, 11 },
	{ -42, 21 },
	{ -128, -128 },
	{ -47, -3 },
	{ -56, 5 },
	{ -55, 14 },
	{ -128, -128 },
	{ 53, 18 },
	{ 58, 15 },
	{ 57, 24 },
	{ -128, -128 },
	{ -6, -3 },
	{ -2, -30 },
	{ 4, -25 },
	{ 13, -44 },
	{ 17, -37 },
	{ 26, -57 },
	{ 18, -29 },
	{ 13, -34 },
	{ 2, -16 },
	{ -1, -22 },
	{ -6, -3 },
};

const struct point game_of_life_icon_points[] = {
	{ -59, 27 },
	{ -60, 55 },
	{ -32, 56 },
	{ -31, 29 },
	{ -57, 25 },
	{ -128, -128 },
	{ -12, 31 },
	{ -13, 55 },
	{ 16, 56 },
	{ 14, 29 },
	{ -13, 30 },
	{ -128, -128 },
	{ 29, 29 },
	{ 29, 54 },
	{ 59, 55 },
	{ 59, 26 },
	{ 27, 26 },
	{ -128, -128 },
	{ 29, 15 },
	{ 59, 16 },
	{ 57, -15 },
	{ 28, -13 },
	{ 29, 14 },
	{ -128, -128 },
	{ 21, -17 },
	{ 18, -46 },
	{ -13, -46 },
	{ -12, -14 },
	{ 19, -16 },
};

const struct point breakout_icon_points[] = {
	{ -64, -52 },
	{ 62, -51 },
	{ -128, -128 },
	{ -63, -41 },
	{ 61, -39 },
	{ -128, -128 },
	{ -63, -28 },
	{ 62, -27 },
	{ -128, -128 },
	{ -62, -20 },
	{ 62, -18 },
	{ -128, -128 },
	{ -48, -52 },
	{ -47, -21 },
	{ -128, -128 },
	{ -24, -52 },
	{ -24, -20 },
	{ -128, -128 },
	{ -3, -51 },
	{ 0, -20 },
	{ -128, -128 },
	{ 20, -49 },
	{ 20, -19 },
	{ -128, -128 },
	{ 40, -48 },
	{ 41, -19 },
	{ -128, -128 },
	{ -30, 52 },
	{ -9, 52 },
	{ -128, -128 },
	{ -35, 28 },
	{ -20, 51 },
	{ 25, 20 },
	{ -128, -128 },
	{ 25, 14 },
	{ 24, 19 },
	{ 28, 19 },
	{ 29, 13 },
	{ 25, 13 },
};

const struct point asteroids_icon_points[] = {
	{ 19, 20 },
	{ 38, 57 },
	{ 42, 42 },
	{ 54, 35 },
	{ 19, 19 },
	{ -128, -128 },
	{ 8, 9 },
	{ -5, -3 },
	{ -128, -128 },
	{ -19, -19 },
	{ -36, -32 },
	{ -128, -128 },
	{ -48, -45 },
	{ -61, -56 },
};

const struct point slotmachine_icon_points[] = {
	{ -30, -55 },
	{ -31, 54 },
	{ 39, 52 },
	{ 32, -53 },
	{ -29, -55 },
	{ -128, -128 },
	{ -24, -48 },
	{ -25, -4 },
	{ 29, -4 },
	{ 27, -46 },
	{ -23, -48 },
	{ -128, -128 },
	{ -9, -47 },
	{ -8, -5 },
	{ -128, -128 },
	{ 11, -47 },
	{ 10, -5 },
	{ -128, -128 },
	{ -40, 32 },
	{ -34, 32 },
	{ -34, -32 },
	{ -39, -35 },
	{ -40, 31 },
	{ -128, -128 },
	{ -43, -34 },
	{ -44, -41 },
	{ -32, -40 },
	{ -32, -32 },
	{ -42, -34 },
	{ -128, -128 },
	{ -17, -35 },
	{ -21, -29 },
	{ -18, -23 },
	{ -13, -30 },
	{ -18, -38 },
	{ -128, -128 },
	{ 0, -37 },
	{ -5, -29 },
	{ 0, -21 },
	{ 6, -30 },
	{ 1, -37 },
	{ -128, -128 },
	{ 18, -36 },
	{ 14, -29 },
	{ 19, -22 },
	{ 24, -29 },
	{ 19, -37 },
};

const struct point clue_icon_points[] = {
	{ -51, 41 },
	{ -42, 46 },
	{ -18, 46 },
	{ -6, 55 },
	{ 11, 61 },
	{ 18, 62 },
	{ 17, 54 },
	{ 6, 35 },
	{ 8, 31 },
	{ 4, 26 },
	{ 6, 23 },
	{ 11, 21 },
	{ 18, 25 },
	{ 23, 23 },
	{ 25, 18 },
	{ 23, 13 },
	{ 27, 17 },
	{ 29, 30 },
	{ 34, 38 },
	{ 43, 40 },
	{ 48, 38 },
	{ 50, 34 },
	{ 56, 32 },
	{ 56, 28 },
	{ 49, 20 },
	{ 39, 19 },
	{ 38, 26 },
	{ 39, 28 },
	{ 38, 29 },
	{ 36, 26 },
	{ 35, 21 },
	{ 34, 17 },
	{ 23, 10 },
	{ 24, 5 },
	{ 27, 0 },
	{ 30, 2 },
	{ 34, 1 },
	{ 29, -10 },
	{ 24, -16 },
	{ 38, -22 },
	{ 41, -27 },
	{ 36, -30 },
	{ 27, -32 },
	{ 22, -46 },
	{ 9, -56 },
	{ 9, -61 },
	{ 0, -61 },
	{ -7, -56 },
	{ -19, -58 },
	{ -29, -52 },
	{ -21, -49 },
	{ -34, -41 },
	{ -39, -35 },
	{ -41, -31 },
	{ -38, -19 },
	{ -42, -12 },
	{ -49, -3 },
	{ -39, -4 },
	{ -34, -6 },
	{ -28, 8 },
	{ -32, 8 },
	{ -44, 23 },
	{ -43, 28 },
	{ -51, 40 },
};

const struct point qc_icon_points[] = {
	{ -37, -11 },
	{ -20, -10 },
	{ -19, 27 },
	{ -37, 26 },
	{ -36, -10 },
	{ -128, -128 },
	{ -20, -5 },
	{ -5, -22 },
	{ 0, -40 },
	{ 5, -43 },
	{ 11, -38 },
	{ 10, -28 },
	{ 5, -14 },
	{ 25, -13 },
	{ 30, -10 },
	{ 32, -3 },
	{ 27, -1 },
	{ 30, 6 },
	{ 24, 11 },
	{ 27, 17 },
	{ 20, 20 },
	{ 22, 27 },
	{ 17, 31 },
	{ -10, 32 },
	{ -18, 24 },
};

const struct point menu_style_icon_points[] = {
	{ -53, -8 },
	{ -49, -3 },
	{ 46, -1 },
	{ 52, -7 },
	{ -128, -128 },
	{ -47, -7 },
	{ -45, -22 },
	{ -32, -39 },
	{ -15, -47 },
	{ 0, -47 },
	{ 14, -46 },
	{ 28, -38 },
	{ 35, -28 },
	{ 42, -12 },
	{ 41, -3 },
	{ -128, -128 },
	{ -38, -20 },
	{ -29, -33 },
	{ -14, -40 },
	{ -128, -128 },
	{ -46, 7 },
	{ -26, 6 },
	{ -128, -128 },
	{ -32, 14 },
	{ 44, 13 },
	{ -128, -128 },
	{ -31, 22 },
	{ 42, 21 },
	{ -128, -128 },
	{ -44, 32 },
	{ -26, 32 },
	{ -128, -128 },
	{ -32, 40 },
	{ 37, 39 },
	{ -128, -128 },
	{ -31, 50 },
	{ 37, 52 },
};

const struct point invert_display_icon_points[] = {
	{ 43, 0 },
	{ 44, -17 },
	{ 36, -33 },
	{ 25, -41 },
	{ 11, -43 },
	{ -9, -43 },
	{ 6, -53 },
	{ -128, -128 },
	{ -9, -43 },
	{ 4, -33 },
	{ -128, -128 },
	{ -38, -1 },
	{ -38, 14 },
	{ -33, 23 },
	{ -24, 34 },
	{ -9, 39 },
	{ 1, 40 },
	{ -11, 49 },
	{ -128, -128 },
	{ 1, 40 },
	{ -9, 28 },
};

const struct point username_icon_points[] = {
	{ -46, -1 },
	{ -45, -22 },
	{ -31, -34 },
	{ -10, -44 },
	{ 1, -43 },
	{ 12, -43 },
	{ 31, -37 },
	{ 40, -27 },
	{ 45, -14 },
	{ 45, -1 },
	{ 43, 31 },
	{ 32, 51 },
	{ 11, 57 },
	{ -14, 56 },
	{ -34, 45 },
	{ -44, 22 },
	{ -46, -1 },
	{ -128, -128 },
	{ -14, 37 },
	{ -4, 40 },
	{ 7, 41 },
	{ 18, 36 },
	{ -128, -128 },
	{ -3, 24 },
	{ 5, 25 },
	{ -128, -128 },
	{ -25, 0 },
	{ -9, 2 },
	{ -128, -128 },
	{ -13, 3 },
	{ -14, 5 },
	{ -128, -128 },
	{ 10, 2 },
	{ 22, -1 },
	{ -128, -128 },
	{ 15, 3 },
	{ 15, 5 },
	{ -128, -128 },
};

const struct point id_icon_points[] = {
	{ -42, -54 },
	{ 44, -55 },
	{ 45, 53 },
	{ -39, 51 },
	{ -42, -52 },
	{ -128, -128 },
	{ 34, -55 },
	{ 36, 51 },
	{ -128, -128 },
	{ 24, -54 },
	{ 26, 53 },
	{ -128, -128 },
	{ -35, -24 },
	{ -17, -24 },
	{ -128, -128 },
	{ -26, -23 },
	{ -27, 18 },
	{ -128, -128 },
	{ -34, 17 },
	{ -19, 18 },
	{ -128, -128 },
	{ -9, -24 },
	{ -11, 17 },
	{ -1, 19 },
	{ 9, 14 },
	{ 12, 0 },
	{ 11, -11 },
	{ 7, -21 },
	{ 1, -23 },
	{ -9, -23 },
};

const struct point screensaver_icon_points[] = {
	{ -59, -41 },
	{ 56, -42 },
	{ 56, 28 },
	{ -57, 28 },
	{ -59, -41 },
	{ -128, -128 },
	{ -10, 28 },
	{ -10, 38 },
	{ -20, 37 },
	{ -22, 45 },
	{ 22, 46 },
	{ 22, 38 },
	{ 11, 38 },
	{ 10, 29 },
	{ -128, -128 },
	{ -34, -25 },
	{ -41, -13 },
	{ -42, -1 },
	{ -37, 11 },
	{ -30, 17 },
	{ -17, 19 },
	{ -9, 16 },
	{ -4, 9 },
	{ -17, 12 },
	{ -25, 7 },
	{ -31, 0 },
	{ -34, -11 },
	{ -34, -25 },
	{ -128, -128 },
	{ -6, -28 },
	{ -6, -16 },
	{ -128, -128 },
	{ -13, -22 },
	{ 1, -23 },
	{ -128, -128 },
	{ 23, -32 },
	{ 23, -26 },
	{ -128, -128 },
	{ 20, -29 },
	{ 25, -29 },
	{ -128, -128 },
	{ 14, -9 },
	{ 15, 4 },
	{ -128, -128 },
	{ 10, -1 },
	{ 19, -3 },
	{ -128, -128 },
	{ 35, 3 },
	{ 35, 16 },
	{ -128, -128 },
	{ 28, 10 },
	{ 41, 11 },
};

const struct point hacker_sim_icon_points[] = {
	{ -32, 45 },
	{ 36, 44 },
	{ 43, 0 },
	{ -43, -1 },
	{ -32, 43 },
	{ -128, -128 },
	{ -24, -4 },
	{ -28, -13 },
	{ -32, -24 },
	{ -28, -34 },
	{ -22, -41 },
	{ -8, -41 },
	{ 9, -42 },
	{ 24, -39 },
	{ 29, -29 },
	{ 28, -17 },
	{ 22, -7 },
	{ 20, 0 },
	{ -128, -128 },
	{ -22, -39 },
	{ -21, -24 },
	{ -16, -7 },
	{ -5, -1 },
	{ 7, -4 },
	{ 15, -13 },
	{ 16, -27 },
	{ 16, -37 },
	{ -128, -128 },
	{ 3, -21 },
	{ 2, -32 },
	{ -128, -128 },
	{ -40, 36 },
	{ -52, 35 },
	{ -57, 20 },
	{ -57, 0 },
	{ -51, -20 },
	{ -43, -28 },
	{ -35, -28 },
	{ -24, -48 },
	{ -1, -62 },
	{ 25, -44 },
	{ 30, -31 },
	{ 32, -27 },
	{ 41, -23 },
	{ 47, -17 },
	{ 51, -6 },
	{ 50, 12 },
	{ 50, 29 },
	{ 44, 38 },
	{ 37, 38 },
};

const struct point moonpatrol_icon_points[] = {
	{ -62, -10 },
	{ -61, -32 },
	{ -14, -33 },
	{ 17, 13 },
	{ 63, -10 },
	{ 14, -38 },
	{ -15, -32 },
	{ -128, -128 },
	{ 2, -12 },
	{ 37, -25 },
	{ -128, -128 },
	{ -4, -12 },
	{ -44, -19 },
	{ -44, -33 },
	{ -128, -128 },
	{ -60, -34 },
	{ -32, -37 },
	{ 12, -38 },
	{ -128, -128 },
	{ -63, -9 },
	{ 17, 18 },
	{ 10, 26 },
	{ 39, 10 },
	{ 62, -9 },
	{ -128, -128 },
	{ 11, -3 },
	{ 18, 8 },
	{ 27, 2 },
	{ 18, -5 },
	{ 12, -4 },
	{ -128, -128 },
	{ 43, -15 },
	{ 46, -17 },
	{ 58, -11 },
	{ 53, -8 },
	{ 44, -14 },
	{ -128, -128 },
	{ -7, 11 },
	{ -15, 13 },
	{ -21, 18 },
	{ -23, 28 },
	{ -22, 36 },
	{ -18, 46 },
	{ -11, 52 },
	{ 2, 49 },
	{ 9, 42 },
	{ 10, 30 },
	{ 5, 18 },
	{ -6, 11 },
	{ -128, -128 },
	{ -19, 19 },
	{ -15, 18 },
	{ -9, 22 },
	{ -4, 31 },
	{ -2, 40 },
	{ -4, 47 },
	{ -7, 50 },
	{ -128, -128 },
	{ -17, 46 },
	{ -12, 46 },
	{ -10, 44 },
	{ -9, 35 },
	{ -12, 29 },
	{ -18, 25 },
	{ -22, 29 },
	{ -128, -128 },
	{ -35, 35 },
	{ -30, 36 },
	{ -25, 33 },
	{ -128, -128 },
	{ -21, 17 },
	{ -24, 11 },
	{ -30, 6 },
	{ -34, 3 },
	{ -40, 5 },
	{ -42, 8 },
	{ -45, 13 },
	{ -44, 21 },
	{ -42, 29 },
	{ -36, 35 },
	{ -128, -128 },
	{ -43, 15 },
	{ -41, 14 },
	{ -38, 15 },
	{ -33, 22 },
	{ -35, 28 },
	{ -38, 31 },
	{ -43, 29 },
	{ -128, -128 },
	{ -55, 22 },
	{ -50, 26 },
	{ -43, 24 },
	{ -128, -128 },
	{ -56, 21 },
	{ -59, 16 },
	{ -62, 5 },
	{ -58, -1 },
	{ -52, -5 },
	{ -42, -1 },
	{ -39, 4 },
	{ -128, -128 },
	{ -60, 7 },
	{ -56, 4 },
	{ -52, 9 },
	{ -53, 18 },
	{ -57, 20 },
	{ -61, 10 },
	{ -128, -128 },
	{ 24, 19 },
	{ 30, 26 },
	{ 38, 28 },
	{ 46, 22 },
	{ 49, 13 },
	{ 45, 5 },
};

const struct point menu_speed_icon_points[] = {
	{ -31, -27 },
	{ -26, -37 },
	{ -18, -41 },
	{ -12, -50 },
	{ -1, -55 },
	{ 2, -56 },
	{ 1, -51 },
	{ 6, -49 },
	{ 3, -41 },
	{ -8, -37 },
	{ -4, -33 },
	{ 3, -35 },
	{ 11, -37 },
	{ 22, -35 },
	{ 23, -40 },
	{ 28, -45 },
	{ 32, -41 },
	{ 35, -33 },
	{ 31, -29 },
	{ 30, -20 },
	{ 26, -16 },
	{ 20, -14 },
	{ 13, -4 },
	{ 2, -3 },
	{ 4, 1 },
	{ 2, 3 },
	{ -4, -2 },
	{ -12, -2 },
	{ -14, -5 },
	{ -10, -9 },
	{ -16, -15 },
	{ -17, -24 },
	{ -26, -24 },
	{ -29, -27 },
	{ -128, -128 },
	{ -21, -34 },
	{ -19, -35 },
	{ -128, -128 },
	{ 0, -16 },
	{ 8, -15 },
	{ 7, -12 },
	{ 3, -10 },
	{ 0, -14 },
	{ 0, -17 },
	{ -128, -128 },
	{ -37, 28 },
	{ -31, 24 },
	{ -26, 28 },
	{ -23, 33 },
	{ -17, 27 },
	{ -3, 20 },
	{ 8, 17 },
	{ 23, 20 },
	{ 38, 32 },
	{ 41, 29 },
	{ 44, 35 },
	{ 37, 38 },
	{ 30, 41 },
	{ 33, 48 },
	{ 30, 51 },
	{ 24, 51 },
	{ 25, 46 },
	{ 22, 42 },
	{ 9, 43 },
	{ -7, 42 },
	{ -11, 50 },
	{ -16, 52 },
	{ -20, 49 },
	{ -17, 47 },
	{ -16, 42 },
	{ -24, 41 },
	{ -29, 39 },
	{ -31, 34 },
	{ -36, 34 },
	{ -37, 29 },
};

#if 0
const struct point bba_icon_points[] = {
	{ -50, -51 },
	{ -46, -16 },
	{ -32, -23 },
	{ -32, -30 },
	{ -35, -36 },
	{ -34, -43 },
	{ -36, -49 },
	{ -42, -53 },
	{ -51, -50 },
	{ -128, -128 },
	{ -42, -47 },
	{ -42, -43 },
	{ -128, -128 },
	{ -41, -32 },
	{ -40, -27 },
	{ -128, -128 },
	{ -35, -47 },
	{ -33, -51 },
	{ -27, -50 },
	{ -21, -43 },
	{ -17, -19 },
	{ -26, -16 },
	{ -25, -28 },
	{ -128, -128 },
	{ -26, -18 },
	{ -35, -14 },
	{ -35, -21 },
	{ -128, -128 },
	{ -28, -43 },
	{ -26, -38 },
	{ -128, -128 },
	{ -21, -45 },
	{ -12, -44 },
	{ -6, -37 },
	{ -7, -27 },
	{ -17, -15 },
	{ -19, -18 },
	{ -128, -128 },
	{ -14, -37 },
	{ -14, -28 },
	{ -128, -128 },
	{ -10, -42 },
	{ -5, -48 },
	{ 1, -49 },
	{ 6, -40 },
	{ 7, -34 },
	{ 1, -33 },
	{ -1, -41 },
	{ -128, -128 },
	{ 2, -34 },
	{ -2, -33 },
	{ -3, -29 },
	{ 1, -27 },
	{ 0, -28 },
	{ -1, -23 },
	{ -128, -128 },
	{ 4, -33 },
	{ 7, -30 },
	{ 5, -21 },
	{ 1, -16 },
	{ -5, -15 },
	{ -9, -24 },
	{ -128, -128 },
	{ 6, -41 },
	{ 15, -41 },
	{ 17, -20 },
	{ 4, -19 },
	{ -128, -128 },
	{ 16, -26 },
	{ 9, -26 },
	{ -128, -128 },
	{ 16, -34 },
	{ 10, -32 },
	{ -128, -128 },
	{ 14, -42 },
	{ 20, -44 },
	{ 24, -36 },
	{ 30, -42 },
	{ 35, -38 },
	{ 28, -28 },
	{ 27, -18 },
	{ 22, -17 },
	{ 21, -27 },
	{ 16, -32 },
	{ -128, -128 },
	{ 36, -46 },
	{ 42, -43 },
	{ 36, -38 },
	{ 36, -46 },
	{ -128, -128 },
	{ 35, -28 },
	{ 34, -36 },
	{ 41, -42 },
	{ 50, -38 },
	{ 50, -33 },
	{ 46, -31 },
	{ 41, -32 },
	{ 46, -30 },
	{ 50, -25 },
	{ 50, -22 },
	{ 46, -15 },
	{ 39, -15 },
	{ 34, -16 },
	{ 32, -20 },
	{ 36, -24 },
	{ 41, -22 },
	{ 35, -30 },
	{ -128, -128 },
	{ -19, -10 },
	{ -18, 15 },
	{ -10, 14 },
	{ -3, 9 },
	{ -4, 4 },
	{ -7, 0 },
	{ -5, -4 },
	{ -5, -11 },
	{ -12, -14 },
	{ -20, -11 },
	{ -128, -128 },
	{ -12, -8 },
	{ -12, -4 },
	{ -128, -128 },
	{ -11, 2 },
	{ -11, 7 },
	{ -128, -128 },
	{ -3, 0 },
	{ -4, -11 },
	{ 6, -10 },
	{ 6, 12 },
	{ -4, 14 },
	{ -5, 10 },
	{ -128, -128 },
	{ -4, 3 },
	{ -3, -1 },
	{ -128, -128 },
	{ 6, -3 },
	{ 9, -8 },
	{ 14, -12 },
	{ 19, -9 },
	{ 22, -1 },
	{ 15, 2 },
	{ 15, -4 },
	{ -128, -128 },
	{ 6, 10 },
	{ 10, 14 },
	{ 17, 14 },
	{ 21, 7 },
	{ 22, 2 },
	{ 14, 1 },
	{ 12, 7 },
	{ 16, 7 },
	{ -128, -128 },
	{ -59, 51 },
	{ -48, 10 },
	{ -42, 15 },
	{ -38, 44 },
	{ -58, 51 },
	{ -128, -128 },
	{ -47, 45 },
	{ -46, 34 },
	{ -128, -128 },
	{ -46, 24 },
	{ -46, 19 },
	{ -128, -128 },
	{ -40, 26 },
	{ -40, 14 },
	{ -35, 14 },
	{ -30, 18 },
	{ -27, 26 },
	{ -29, 37 },
	{ -34, 44 },
	{ -40, 47 },
	{ -41, 44 },
	{ -128, -128 },
	{ -34, 33 },
	{ -34, 22 },
	{ -128, -128 },
	{ -29, 36 },
	{ -24, 49 },
	{ -18, 36 },
	{ -16, 20 },
	{ -28, 20 },
	{ -128, -128 },
	{ -23, 21 },
	{ -23, 32 },
	{ -128, -128 },
	{ -20, 40 },
	{ -19, 43 },
	{ -9, 42 },
	{ -8, 19 },
	{ -15, 20 },
	{ -128, -128 },
	{ -8, 25 },
	{ -13, 26 },
	{ -128, -128 },
	{ -9, 34 },
	{ -13, 33 },
	{ -128, -128 },
	{ -8, 39 },
	{ -3, 40 },
	{ -1, 26 },
	{ -1, 41 },
	{ 8, 43 },
	{ 12, 35 },
	{ 10, 20 },
	{ 3, 21 },
	{ 6, 31 },
	{ 5, 34 },
	{ -128, -128 },
	{ 3, 21 },
	{ -2, 17 },
	{ -7, 19 },
	{ -128, -128 },
	{ 12, 28 },
	{ 12, 35 },
	{ 12, 43 },
	{ 18, 45 },
	{ 18, 28 },
	{ 22, 28 },
	{ 23, 17 },
	{ 9, 20 },
	{ -128, -128 },
	{ 19, 40 },
	{ 23, 45 },
	{ 28, 46 },
	{ 32, 40 },
	{ 33, 19 },
	{ 23, 19 },
	{ -128, -128 },
	{ 26, 19 },
	{ 25, 34 },
	{ -128, -128 },
	{ 33, 21 },
	{ 40, 20 },
	{ 44, 22 },
	{ 46, 28 },
	{ 43, 35 },
	{ 47, 47 },
	{ 41, 49 },
	{ 38, 38 },
	{ 38, 47 },
	{ 34, 48 },
	{ 31, 39 },
	{ -128, -128 },
	{ 38, 27 },
	{ 38, 30 },
	{ -128, -128 },
	{ 45, 24 },
	{ 57, 20 },
	{ 56, 29 },
	{ 55, 35 },
	{ 57, 42 },
	{ 46, 45 },
	{ -128, -128 },
	{ 55, 35 },
	{ 49, 37 },
	{ -128, -128 },
	{ 56, 27 },
	{ 50, 30 },
};
#endif

struct menu_icon games_icon = {
	.npoints = ARRAYSIZE(games_icon_points),
	.color = GREEN,
	.points = &games_icon_points[0],
};

struct menu_icon settings_icon = {
	.npoints = ARRAYSIZE(settings_icon_points),
	.color = GREEN,
	.points = &settings_icon_points[0],
};

struct menu_icon schedule_icon = {
	.npoints = ARRAYSIZE(schedule_icon_points),
	.color = GREEN,
	.points = &schedule_icon_points[0],
};

struct menu_icon about_icon = {
	.npoints = ARRAYSIZE(about_icon_points),
	.color = GREEN,
	.points = &about_icon_points[0],
};

struct menu_icon battlezone_icon = {
	.npoints = ARRAYSIZE(battlezone_icon_points),
	.color = GREEN,
	.points = &battlezone_icon_points[0],
};

struct menu_icon lunar_rescue_icon = {
	.npoints = ARRAYSIZE(lunar_rescue_icon_points),
	.color = GREEN,
	.points = &lunar_rescue_icon_points[0],
};

struct menu_icon legacy_games_icon = {
	.npoints = ARRAYSIZE(legacy_games_icon_points),
	.color = GREEN,
	.points = &legacy_games_icon_points[0],
};

struct menu_icon badge_monsters_icon = {
	.npoints = ARRAYSIZE(badge_monsters_icon_points),
	.color = GREEN,
	.points = &badge_monsters_icon_points[0],
};

struct menu_icon backlight_icon = {
	.npoints = ARRAYSIZE(backlight_icon_points),
	.color = GREEN,
	.points = &backlight_icon_points[0],
};

struct menu_icon led_icon = {
	.npoints = ARRAYSIZE(led_icon_points),
	.color = GREEN,
	.points = &led_icon_points[0],
};

struct menu_icon audio_icon = {
	.npoints = ARRAYSIZE(audio_icon_points),
	.color = GREEN,
	.points = &audio_icon_points[0],
};

struct menu_icon clear_nvram_icon = {
	.npoints = ARRAYSIZE(clear_nvram_icon_points),
	.color = GREEN,
	.points = &clear_nvram_icon_points[0],
};

struct menu_icon game_of_life_icon = {
	.npoints = ARRAYSIZE(game_of_life_icon_points),
	.color = GREEN,
	.points = &game_of_life_icon_points[0],
};

struct menu_icon breakout_icon = {
	.npoints = ARRAYSIZE(breakout_icon_points),
	.color = GREEN,
	.points = &breakout_icon_points[0],
};

struct menu_icon asteroids_icon = {
	.npoints = ARRAYSIZE(asteroids_icon_points),
	.color = GREEN,
	.points = &asteroids_icon_points[0],
};

struct menu_icon slotmachine_icon = {
	.npoints = ARRAYSIZE(slotmachine_icon_points),
	.color = GREEN,
	.points = &slotmachine_icon_points[0],
};

struct menu_icon clue_icon = {
	.npoints = ARRAYSIZE(clue_icon_points),
	.color = GREEN,
	.points = &clue_icon_points[0],
};

struct menu_icon qc_icon = {
	.npoints = ARRAYSIZE(qc_icon_points),
	.color = GREEN,
	.points = &qc_icon_points[0],
};

struct menu_icon menu_style_icon = {
	.npoints = ARRAYSIZE(menu_style_icon_points),
	.color = GREEN,
	.points = &menu_style_icon_points[0],
};

struct menu_icon invert_display_icon = {
	.npoints = ARRAYSIZE(invert_display_icon_points),
	.color = GREEN,
	.points = &invert_display_icon_points[0],
};

struct menu_icon username_icon = {
	.npoints = ARRAYSIZE(username_icon_points),
	.color = GREEN,
	.points = &username_icon_points[0],
};

struct menu_icon id_icon = {
	.npoints = ARRAYSIZE(id_icon_points),
	.color = GREEN,
	.points = &id_icon_points[0],
};

struct menu_icon screensaver_icon = {
	.npoints = ARRAYSIZE(screensaver_icon_points),
	.color = GREEN,
	.points = &screensaver_icon_points[0],
};

struct menu_icon hacker_sim_icon = {
	.npoints = ARRAYSIZE(hacker_sim_icon_points),
	.color = GREEN,
	.points = &hacker_sim_icon_points[0],
};

struct menu_icon moonpatrol_icon = {
	.npoints = ARRAYSIZE(moonpatrol_icon_points),
	.color = GREEN,
	.points = &moonpatrol_icon_points[0],
};

struct menu_icon menu_speed_icon = {
	.npoints = ARRAYSIZE(menu_speed_icon_points),
	.color = GREEN,
	.points = &menu_speed_icon_points[0],
};

#if 0
struct menu_icon bba_icon = {
	.npoints = ARRAYSIZE(bba_icon_points),
	.color = GREEN,
	.points = &bba_icon_points[0],
};
#endif

#endif
