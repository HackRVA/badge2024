//
// Created by Samuel Jones on 2/21/22.
//

#include "init.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <SDL.h>
#include "framebuffer.h"
#include "display_s6b33.h"
#include "led_pwm.h"
#include "button.h"
#include "button_sdl_ui.h"
#include "ir.h"
#include "rtc.h"
#include "flash_storage.h"
#include "led_pwm_sdl.h"

#define UNUSED __attribute__((unused))

static int sim_argc;
static char** sim_argv;

// Forward declaration
void hal_start_sdl(int *argc, char ***argv);

// Do hardware-specific initialization.
void hal_init(void) {

    S6B33_init_gpio();
    led_pwm_init_gpio();
    button_init_gpio();
    ir_init();
    S6B33_reset();
    rtc_init_badge(0);
}

void *main_in_thread(void* params) {
    int (*main_func)(int, char**) = params;
    main_func(sim_argc, sim_argv);
    return NULL;
}

int hal_run_main(int (*main_func)(int, char**), int argc, char** argv) {

    sim_argc = argc;
    sim_argv = argv;

    pthread_t app_thread;
    pthread_create(&app_thread, NULL, main_in_thread, main_func);

    // Should not return until GTK exits.
    hal_start_sdl(&argc, &argv);

    return 0;
}

void hal_deinit(void) {
    flash_deinit();
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}

void hal_reboot(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    exit(0);
}

uint32_t hal_disable_interrupts(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return 0;
}

void hal_restore_interrupts(__attribute__((unused)) uint32_t state) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}


// static GtkWidget *vbox, *drawing_area;
static SDL_Window *window;
static SDL_Renderer *renderer;
#define SCALE_FACTOR 6
#define EXTRA_WIDTH 200
#define SIM_SCREEN_WIDTH (LCD_XSIZE * SCALE_FACTOR + EXTRA_WIDTH)
#define SIM_SCREEN_HEIGHT (LCD_YSIZE * SCALE_FACTOR)
static int real_screen_width = SIM_SCREEN_WIDTH;
static int real_screen_height = SIM_SCREEN_HEIGHT;
static SDL_Texture *pix_buf;
static char *program_title;
extern int lcd_brightness;

static void draw_led_text(SDL_Renderer *renderer, int x, int y)
{
#define LETTER_SPACING 12
    /* Literally draws L E D */
    /* Draw L */
    SDL_RenderDrawLine(renderer, x, y, x, y - 10);
    SDL_RenderDrawLine(renderer, x, y, x + 8, y);

    x += LETTER_SPACING;

    /* Draw E */
    SDL_RenderDrawLine(renderer, x, y, x, y - 10);
    SDL_RenderDrawLine(renderer, x, y, x + 8, y);
    SDL_RenderDrawLine(renderer, x, y - 5, x + 5, y - 5);
    SDL_RenderDrawLine(renderer, x, y - 10, x + 8, y - 10);

    x += LETTER_SPACING;

    /* Draw D */
    SDL_RenderDrawLine(renderer, x, y, x, y - 10);
    SDL_RenderDrawLine(renderer, x, y, x + 8, y);
    SDL_RenderDrawLine(renderer, x, y - 10, x + 8, y - 10);
    SDL_RenderDrawLine(renderer, x + 8, y - 10, x + 10, y - 5);
    SDL_RenderDrawLine(renderer, x + 8, y, x + 10, y - 5);
}

void flareled(unsigned char r, unsigned char g, unsigned char b)
{
    led_color.red = r;
    led_color.green = g;
    led_color.blue = b;
}


/* TODO: I should not need display_array_with_alpha[] but SDL_PIXELFORMAT_RGB888 seems
 * to require alpha despite the name, or... there's some missing piece of the puzzle
 * that remains to be found.
 */

static int draw_window(SDL_Renderer *renderer, SDL_Texture *texture)
{
    extern uint8_t display_array[LCD_YSIZE][LCD_XSIZE][3];


    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    /* Draw the pixels of the screen */


    /* This:
     *
     *    SDL_UpdateTexture(texture, NULL, display_array, LCD_XSIZE * 3);
     *
     * doesn't work right for some reason I don't quite understand, having to do with
     * the alpha channel even though I tried to tell it we don't have an alpha channel.
     * I think what may be happening is that the SDL_PIXELFORMAT_RGB888
     * may be referring to the format of the texture (after it's copied)
     * rather than the format of the data you're copying in?
     *
     * In any case, for now, we can copy display_array inserting the
     * alpha channel that SDL_RenderCopy seems to expect. Modern
     * computers can do this copy in microseconds, so it's not a big deal.
     */

    /* Copy display_array[] but add on an alpha channel. SDL_RenderCopy() seems to need it.
     * Plus we try to use it to implement LCD brightness. */
    static uint8_t display_array_with_alpha[LCD_YSIZE][LCD_XSIZE][4];

    for (int y = 0; y < LCD_YSIZE; y++) {
        for (int x = 0; x < LCD_XSIZE; x++) {
            display_array_with_alpha[y][x][0] = display_array[y][x][0];
            display_array_with_alpha[y][x][1] = display_array[y][x][1];
            display_array_with_alpha[y][x][2] = display_array[y][x][2];
	    /* We can try to implement lcd brightness via alpha channel, but it doesn't seem to work */
            display_array_with_alpha[y][x][3] = 255 - lcd_brightness;
        }
    }
    SDL_UpdateTexture(texture, NULL, display_array_with_alpha, LCD_XSIZE * 4);
    SDL_RenderCopy(renderer, texture, &(SDL_Rect) { 0, 0, LCD_XSIZE, LCD_YSIZE },
                                      &(SDL_Rect) { 0, 0, LCD_XSIZE * SCALE_FACTOR, LCD_YSIZE * SCALE_FACTOR});

#if 0
    /* Another attempt to implement LCD brightness by alpha blending a black rectangle over
     * the "screen", which also doesn't actually work, probably for the same reason. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255 - lcd_brightness);
    SDL_RenderFillRect(renderer, &(SDL_Rect) { 0, 0, SCALE_FACTOR * LCD_XSIZE, SCALE_FACTOR * LCD_YSIZE });
#endif

    int x, y, w, h;
    w = (real_screen_width - EXTRA_WIDTH) / LCD_XSIZE;
    if (w < 1)
        w = 1;
    h = real_screen_height / LCD_YSIZE;
    if (h < 1)
        h = 1;

    /* Draw a white vertical line demarcating the right edge of the screen */
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderDrawLine(renderer, LCD_XSIZE * w + 1, 0, LCD_XSIZE * w + 1, real_screen_height - 1);

    /* Draw simulated flare LED */
    x = LCD_XSIZE * w + EXTRA_WIDTH / 4;
    y = (LCD_YSIZE * h) / 2 - EXTRA_WIDTH / 4;
    draw_led_text(renderer, LCD_XSIZE * w + EXTRA_WIDTH / 2 - 20, y - 10);
    SDL_SetRenderDrawColor(renderer, led_color.red, led_color.blue, led_color.green, 0xff);
    SDL_RenderFillRect(renderer, &(SDL_Rect) { x, y, EXTRA_WIDTH / 2, EXTRA_WIDTH / 2} );
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderDrawRect(renderer, &(SDL_Rect) { x, y, EXTRA_WIDTH / 2, EXTRA_WIDTH / 2} );

    SDL_RenderPresent(renderer);
    return 0;
}

static int start_sdl(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Unable to initialize SDL (Video):  %s\n", SDL_GetError());
        return 1;
    }
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "Unable to initialize SDL (Events):  %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);
    return 0;
}

static void setup_window_and_renderer(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture)
{
    char window_title[1024];

    snprintf(window_title, sizeof(window_title), "HackRVA Badge Emulator - %s", program_title);
    free(program_title);
    *window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               0, 0, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (!*window) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetWindowSize(*window, SIM_SCREEN_WIDTH, SIM_SCREEN_HEIGHT);
    *renderer = SDL_CreateRenderer(*window, -1, 0);
    if (!*renderer) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, LCD_XSIZE, LCD_YSIZE);
    if (!*texture) { 
        fprintf(stderr, "Could not create texture: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
    SDL_ShowWindow(*window);
    SDL_RenderClear(*renderer);
    SDL_RenderPresent(*renderer);
}

static void process_events(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            key_press_cb(&event.key.keysym);
            break;
        case SDL_KEYUP:
            key_release_cb(&event.key.keysym);
            break;
        case SDL_QUIT:
            /* Handle quit requests (like Ctrl-c). */
            time_to_quit = 1;
            break;
        case SDL_WINDOWEVENT:
            break;
        case SDL_MOUSEBUTTONDOWN:
            break;
        case SDL_MOUSEBUTTONUP:
            break;
        case SDL_MOUSEMOTION:
            break;
        case SDL_MOUSEWHEEL:
            break;
        }
    }
}

static void wait_until_next_frame(void)
{
    static uint32_t next_frame = 0;

    if (next_frame == 0)
        next_frame = SDL_GetTicks() + 33;  /* 30 Hz */
    uint32_t now = SDL_GetTicks();
    if (now < next_frame)
        SDL_Delay(next_frame - now);
    next_frame += 33; /* 30 Hz */
}

void hal_start_sdl(UNUSED int *argc, UNUSED char ***argv)
{
    program_title = strdup((*argv)[0]);
    if (start_sdl())
	exit(1);
    setup_window_and_renderer(&window, &renderer, &pix_buf);
    flareled(0, 0, 0);

    while (!time_to_quit) {
	draw_window(renderer, pix_buf);
	process_events();
	wait_until_next_frame();
    }

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();

    printf("\n\n\n\n\n\n\n\n\n\n\n");
    printf("If you seak leak sanitizer complaining about memory and _XlcDefaultMapModifiers\n");
    printf("it's because SDL is programmed by monkeys.\n");
    printf("\n\n\n\n\n\n\n\n\n\n\n");
    exit(0);
}
