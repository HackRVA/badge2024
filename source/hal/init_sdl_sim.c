//
// Created by Samuel Jones on 2/21/22.
//

#include "init.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <SDL.h>
#include <SDL_image.h>
#include "framebuffer.h"
#include "display_s6b33.h"
#include "led_pwm.h"
#include "button.h"
#include "button_coords.h"
#include "button_sdl_ui.h"
#include "ir.h"
#include "rtc.h"
#include "flash_storage.h"
#include "led_pwm_sdl.h"
#include "sim_lcd_params.h"
#include "png_utils.h"
#include "trig.h"

#define UNUSED __attribute__((unused))

static int sim_argc;
static char** sim_argv;
static int fullscreen = 0;

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

static char *badge_image_pixels, *landscape_badge_image_pixels;
static int badge_image_width, badge_image_height;
static int landscape_badge_image_width, landscape_badge_image_height;

// static GtkWidget *vbox, *drawing_area;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *pix_buf, *landscape_pix_buf, *badge_image, *landscape_badge_image;
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

static void draw_badge_image(struct sim_lcd_params *slp)
{
	static int created_textures = 0;
	float bx1, by1, bx2, by2;
	float cx1, cy1, cx2, cy2;
	float sx1, sy1, sx2, sy2;
	int sx, sy;
	struct lcd_to_circuit_board_relation lcdp;

	if (!created_textures) {
		if (landscape_badge_image_pixels) {
			landscape_badge_image = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
					landscape_badge_image_width, landscape_badge_image_height);
			SDL_UpdateTexture(landscape_badge_image, NULL, landscape_badge_image_pixels, landscape_badge_image_width * 4);
		}
		if (badge_image_pixels) {
			badge_image = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
							badge_image_width, badge_image_height);
			SDL_UpdateTexture(badge_image, NULL, badge_image_pixels, badge_image_width * 4);
		}
		created_textures = 1;
	}

        SDL_GetWindowSize(window, &sx, &sy);

	/* get corners of the screen inside the badge image */
	if (slp->orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
		lcdp = landscape_lcd_to_board();
	else
		lcdp = portrait_lcd_to_board();

	/* corners of the sim screen on the computer screen */
	sx1 = slp->xoffset;
	sy1 = slp->yoffset;
	sx2 = slp->xoffset + slp->width;
	sy2 = slp->yoffset + slp->height;

	/* where corners of badge image land on screen, by similar triangles */
	float fx = (sx2 - sx1) / (lcdp.x2 - lcdp.x1);
	float fy = (sy2 - sy1) / (lcdp.y2 - lcdp.y1);
	fx = fy;
	bx1 =   sx1 - fx * lcdp.x1;
	by1 =   sy1 - fy * lcdp.y1;
	if (slp->orientation == SIM_LCD_ORIENTATION_LANDSCAPE) {
		bx2 =   bx1 + fx * (landscape_badge_image_width - 1);
		by2 =   by1 + fy * (landscape_badge_image_height - 1);
	} else {
		bx2 =   bx1 + fx * (badge_image_width - 1);
		by2 =   by1 + fy * (badge_image_height - 1);
	}

	/* note, some or all of bx1, by1, bx2, by2 may be off screen. Compute clip rect */
	if (bx1 < 0) {
		cx1 = -bx1 / fx;
		bx1 = 0;
	} else {
		cx1 = 0;
	}
	if (by1 < 0) {
		cy1 = -by1 / fy;
		by1 = 0;
	} else {
		cy1 = 0;
	}
	if (bx2 >= sx) {
		cx2 = (sx - bx1) / fx;
		bx2 = sx - 1;
	} else {
		if (slp->orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
			cx2 = landscape_badge_image_width - 1;
		else
			cx2 = badge_image_width - 1;
	}
	if (by2 >= sy) {
		cy2 = (sy - by1) / fy;
		by2 = sy - 1;
	} else {
		if (slp->orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
			cy2 = landscape_badge_image_height - 1;
		else
			cy2 = badge_image_height - 1;
	}

	SDL_Rect from_rect = { (int) cx1, (int) cy1, (int) (cx2 - cx1), (int) (cy2 - cy1) };
	SDL_Rect to_rect = { (int) bx1, (int) by1, (int) (bx2 - bx1), (int) (by2 - by1) };
	if (slp->orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
		SDL_RenderCopy(renderer, landscape_badge_image, &from_rect, &to_rect);
	else
		SDL_RenderCopy(renderer, badge_image, &from_rect, &to_rect);
}

static void draw_button_press(struct button_coord *b)
{
    SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);
    SDL_RenderFillRect(renderer, &(SDL_Rect) { b->x - 20, b->y - 20, 40, 40} );
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderFillRect(renderer, &(SDL_Rect) { b->x - 10, b->y - 10, 20, 20} );
}

static void draw_rotary_button_position(struct button_coord *button, int which_rotary)
{
	float x, y;
	int angle = sim_get_rotary_angle(which_rotary);

	x = (cosine(angle) * 40) >> 8;
	y = (sine(angle) * 40) >> 8;

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
	SDL_RenderDrawLine(renderer, button->x, button->y, button->x + x, button->y + y);
}

static void draw_button_inputs(struct sim_lcd_params *slp)
{
	struct sim_button_status button_status;
	int w, h;

	if (slp->orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
		w = badge_image_width;
		h = badge_image_height;
	} else {
		w = landscape_badge_image_width;
		h = landscape_badge_image_height;
	}
	struct button_coord_list bcl = get_button_coords(slp, w, h);
	button_status = get_sim_button_status();

	/* TODO: now there are 4 buttons, A, B, and the two rotary switches */
	if (button_status.button_a)
		draw_button_press(&bcl.a_button);
	if (button_status.button_b)
		draw_button_press(&bcl.b_button);
	if (button_status.dpad_right)
		draw_button_press(&bcl.dpad_right);
	if (button_status.dpad_left)
		draw_button_press(&bcl.dpad_left);
	if (button_status.dpad_up)
		draw_button_press(&bcl.dpad_up);
	if (button_status.dpad_down)
		draw_button_press(&bcl.dpad_down);
	if (button_status.left_rotary_button)
		draw_button_press(&bcl.left_rotary);
	if (button_status.right_rotary_button)
		draw_button_press(&bcl.right_rotary);
	draw_rotary_button_position(&bcl.right_rotary, 0);
	draw_rotary_button_position(&bcl.left_rotary, 1);
	sim_button_status_countdown();
}

static int draw_window(SDL_Renderer *renderer, SDL_Texture *texture, SDL_Texture *landscape_texture)
{
    extern uint8_t display_array[LCD_YSIZE][LCD_XSIZE][3];
    struct sim_lcd_params slp = get_sim_lcd_params();

    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderClear(renderer);

    draw_badge_image(&slp);
    draw_button_inputs(&slp);

    /* Draw the pixels of the screen */


    /* This:
     *
     *    SDL_UpdateTexture(texture, NULL, display_array, LCD_XSIZE * 3);
     *
     * doesn't work right for some reason I don't quite.  The texture apparently
     * wants the data in BGRA order.
     *
     * In any case, for now, we can copy display_array inserting the
     * alpha channel that SDL_RenderCopy seems to expect. Modern
     * computers can do this copy in microseconds, so it's not a big deal.
     */

    /* Copy display_array[] but add on an alpha channel. SDL_RenderCopy() seems to need it.
     * Plus we try to use it to implement LCD brightness. */
    static uint8_t display_array_with_alpha[LCD_YSIZE][LCD_XSIZE][4];
    static uint8_t landscape_display_array_with_alpha[LCD_XSIZE][LCD_YSIZE][4];

    if (slp.orientation == SIM_LCD_ORIENTATION_PORTRAIT) { /* LCD screen orientation */
        float level = (float) lcd_brightness / 255.0f;
        for (int y = 0; y < LCD_YSIZE; y++) {
            for (int x = 0; x < LCD_XSIZE; x++) {
                /* SDL texture seems to want data in BGRA order, and since we're copying
                 * anyway, we can emulate LCD brightness here too. */
                display_array_with_alpha[y][x][2] = (uint8_t) (level * display_array[y][x][0]);
                display_array_with_alpha[y][x][1] = (uint8_t) (level * display_array[y][x][1]);
                display_array_with_alpha[y][x][0] = (uint8_t) (level * display_array[y][x][2]);
	        /* I tried to implement lcd brightness via alpha channel, but it doesn't seem to work */
                /* display_array_with_alpha[y][x][3] = 255 - lcd_brightness; */
                display_array_with_alpha[y][x][3] = 255;
            }
        }
        SDL_UpdateTexture(texture, NULL, display_array_with_alpha, LCD_XSIZE * 4);
        SDL_Rect from_rect = { 0, 0, LCD_XSIZE, LCD_YSIZE };
        SDL_Rect to_rect = { slp.xoffset, slp.yoffset, slp.width, slp.height };
        SDL_RenderCopy(renderer, texture, &from_rect, &to_rect);
    } else { /* landscape */
        float level = (float) lcd_brightness / 255.0f;
        for (int x = 0; x < LCD_XSIZE; x++) {
            for (int y = 0; y < LCD_YSIZE; y++) {
                /* SDL texture seems to want data in BGRA order, and since we're copying
                 * anyway, we can emulate LCD brightness here too. */
                landscape_display_array_with_alpha[LCD_XSIZE - x - 1][y][2] = (uint8_t) (level * display_array[y][x][0]);
                landscape_display_array_with_alpha[LCD_XSIZE - x - 1][y][1] = (uint8_t) (level * display_array[y][x][1]);
                landscape_display_array_with_alpha[LCD_XSIZE - x - 1][y][0] = (uint8_t) (level * display_array[y][x][2]);
	        /* I tried to implement lcd brightness via alpha channel, but it doesn't seem to work */
                /* display_array_with_alpha[y][x][3] = 255 - lcd_brightness; */
                landscape_display_array_with_alpha[x][y][3] = 255;
            }
        }
        SDL_UpdateTexture(landscape_texture, NULL, landscape_display_array_with_alpha, LCD_YSIZE * 4);
        SDL_Rect from_rect = { 0, 0, LCD_YSIZE, LCD_XSIZE };
        SDL_Rect to_rect = { slp.xoffset, slp.yoffset, slp.width, slp.height };
        SDL_RenderCopy(renderer, landscape_texture, &from_rect, &to_rect);
    }

    int x, y;

    /* Draw a border around the simulated screen */
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderDrawLine(renderer, slp.xoffset - 1, slp.yoffset - 1., slp.xoffset + slp.width + 1, slp.yoffset - 1); /* top */
    SDL_RenderDrawLine(renderer, slp.xoffset - 1, slp.yoffset - 1., slp.xoffset - 1, slp.yoffset + slp.height + 1); /* left */
    SDL_RenderDrawLine(renderer, slp.xoffset - 1, slp.yoffset + slp.height + 1, slp.xoffset + slp.width + 1, slp.yoffset + slp.height + 1); /* bottom */
    SDL_RenderDrawLine(renderer, slp.xoffset + slp.width + 1, slp.yoffset - 1, slp.xoffset + slp.width + 1, slp.yoffset + slp.height + 1); /* right */

    /* Draw simulated flare LED */
    x = slp.xoffset + slp.width + 20;
    y = slp.yoffset + (slp.height / 2) - 20;
    draw_led_text(renderer, x, y);
    SDL_SetRenderDrawColor(renderer, led_color.red, led_color.blue, led_color.green, 0xff);
    SDL_RenderFillRect(renderer, &(SDL_Rect) { x, y + 20, 51, 51} );
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderDrawRect(renderer, &(SDL_Rect) { x, y + 20, 50, 50} );

    SDL_RenderPresent(renderer);
    return 0;
}

static void enable_sdl_fullscreen_sanity(void)
{
	/* If SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS isn't set to zero,
	 * fullscreen window behavior is *insane* by default.
	 *
	 * Alt-tab and Alt-left-arrow and Alt-right-arrow will *minimize*
	 * the window, pushing it to the bottom of the stack, so when you
	 * alt-tab again, and expect the window to re-appear, it doesn't.
	 * Instead, a different window appears, and you have to alt-tab a
	 * zillion times through all your windows until you finally get to
	 * the bottom where your minimized fullscreen window sits, idiotically.
	 *
	 * Let's make sanity the default.  The last parameter of setenv()
	 * says do not overwrite the value if it is already set. This will
	 * allow for any completely insane individuals who somehow prefer
	 * this idiotc behavior to still have it.  But they will not get
	 * it by default.
	 */

	char *v = getenv("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS");
	if (v && strncmp(v, "1", 1) == 0) {
		fprintf(stderr, "You have SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS set to 1!\n");
		fprintf(stderr, "I highly recommend you set it to zero. But it's your sanity\n");
		fprintf(stderr, "at stake, not mine, so whatever. Let's proceed anyway.\n");
	}
	setenv("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0", 0);	/* Final 0 means don't override user's prefs */
								/* I am Very tempted to set it to 1. */
}

static int start_sdl(void)
{
    enable_sdl_fullscreen_sanity();
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

static void load_badge_images(void)
{
	int w = 0, h = 0, a = 0;
	char whynot[1024];

	badge_image_pixels = png_utils_read_png_image("../images/badge-image-1024.png",
		0, 0, 0, &w, &h, &a, whynot, sizeof(whynot) - 1);
	if (!badge_image_pixels)
		fprintf(stderr, "Failed to load badge image: %s\n", whynot);
	badge_image_width = w;
	badge_image_height = h;

        whynot[0] = '\0';
	w = 0;
	h = 0;
	a = 0;
	landscape_badge_image_pixels = png_utils_read_png_image("../images/badge-image-vert-1024.png",
		0, 0, 0, &w, &h, &a, whynot, sizeof(whynot) - 1);
	if (!landscape_badge_image_pixels)
		fprintf(stderr, "Failed to load landscape badge image: %s\n", whynot);
	landscape_badge_image_width = w;
	landscape_badge_image_height = h;
}

void toggle_fullscreen_mode(void)
{
	if (fullscreen)
		SDL_SetWindowFullscreen(window, 0);
	else
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	fullscreen = !fullscreen;
	/* configure event takes care of resizing window */
}

static void setup_window_and_renderer(SDL_Window **window, SDL_Renderer **renderer,
				SDL_Texture **texture, SDL_Texture **landscape_texture)
{
    char window_title[1024];

    load_badge_images();
    snprintf(window_title, sizeof(window_title), "HackRVA Badge Emulator - %s", program_title);
    free(program_title);
    *window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               0, 0, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (!*window) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetWindowSize(*window, 1000, 800);
    // SDL_SetWindowFullscreen(*window, SDL_WINDOW_FULLSCREEN_DESKTOP);

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
    *landscape_texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, LCD_YSIZE, LCD_XSIZE);
    if (!*landscape_texture) { 
        fprintf(stderr, "Could not create landscape texture: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetTextureBlendMode(*landscape_texture, SDL_BLENDMODE_BLEND);

    SDL_ShowWindow(*window);
    SDL_RenderClear(*renderer);
    SDL_RenderPresent(*renderer);
}

static void process_events(SDL_Window *window)
{
    SDL_Event event;
    struct button_coord_list bcl;
    struct sim_lcd_params slp;
    int w, h;

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
            handle_window_event(window, event);
            break;
        case SDL_MOUSEBUTTONDOWN:
            slp = get_sim_lcd_params();
            if (slp.orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
                w = badge_image_width;
                h = badge_image_height;
            } else {
                w = landscape_badge_image_width;
                h = landscape_badge_image_height;
            }
            bcl = get_button_coords(&slp, w, h);
            mouse_button_down_cb(&event, &bcl);
            break;
        case SDL_MOUSEBUTTONUP:
            break;
        case SDL_MOUSEMOTION:
            break;
        case SDL_MOUSEWHEEL:
            slp = get_sim_lcd_params();
            if (slp.orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
                w = badge_image_width;
                h = badge_image_height;
            } else {
                w = landscape_badge_image_width;
                h = landscape_badge_image_height;
            }
            bcl = get_button_coords(&slp, w, h);
            mouse_scroll_cb(&event, &bcl);
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
    int first_time = 1;

    program_title = strdup((*argv)[0]);
    if (start_sdl())
	exit(1);
    setup_window_and_renderer(&window, &renderer, &pix_buf, &landscape_pix_buf);
    flareled(0, 0, 0);

    while (!time_to_quit) {
	draw_window(renderer, pix_buf, landscape_pix_buf);

	if (first_time) {
            int sx, sy;
            SDL_GetWindowSize(window, &sx, &sy);
            adjust_sim_lcd_params_defaults(sx, sy);
            set_sim_lcd_params_default();
            first_time = 0;
        }

	process_events(window);
	wait_until_next_frame();
    }

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    free(badge_image_pixels);
    free(landscape_badge_image_pixels);
    SDL_Quit();


    printf("\n\n\n\n\n\n\n\n\n\n\n");
    printf("If you seak leak sanitizer complaining about memory and _XlcDefaultMapModifiers\n");
    printf("it's because SDL is programmed by monkeys.\n");
    printf("\n\n\n\n\n\n\n\n\n\n\n");
    exit(0);
}
