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
#include "display.h"
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
#include "quat.h"

#define UNUSED __attribute__((unused))
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

static int sim_argc;
static char** sim_argv;
static int fullscreen = 0;

// Forward declaration
void hal_start_sdl(int *argc, char ***argv);

// Do hardware-specific initialization.
void hal_init(void) {

    display_init_gpio();
    led_pwm_init_gpio();
    button_init_gpio();
    ir_init();
    display_reset();
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

static char *badge_image_pixels, *landscape_badge_image_pixels, *badge_background_pixels;
static char *quit_confirm_pixels;
int quit_confirm_active = 0;
static char *led_pixels;
static int badge_image_width, badge_image_height;
static int landscape_badge_image_width, landscape_badge_image_height;
static int badge_background_width, badge_background_height;
static int quit_confirm_width, quit_confirm_height;
static int led_width, led_height;
static SDL_Joystick *joystick = NULL;

// static GtkWidget *vbox, *drawing_area;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *pix_buf, *landscape_pix_buf, *badge_image, *landscape_badge_image, *badge_background_image;
static SDL_Texture *led_image;
static SDL_Texture *quit_confirm_image;
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

static void draw_badge_background(void)
{
	SDL_RenderCopy(renderer, badge_background_image, NULL, NULL);
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
		if (badge_background_pixels) {
			badge_background_image = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
							badge_background_width, badge_background_height);
			SDL_UpdateTexture(badge_background_image, NULL, badge_background_pixels, badge_background_width * 4);
		}
		if (led_pixels) {
			led_image = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
							led_width, led_height);
			SDL_SetTextureBlendMode(led_image, SDL_BLENDMODE_BLEND);
			SDL_UpdateTexture(led_image, NULL, led_pixels, led_width * 4);
		}
		if (quit_confirm_pixels) {
			quit_confirm_image = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
							quit_confirm_width, quit_confirm_height);
			SDL_SetTextureBlendMode(quit_confirm_image, SDL_BLENDMODE_BLEND);
			SDL_UpdateTexture(quit_confirm_image, NULL, quit_confirm_pixels, quit_confirm_width * 4);
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

static void draw_flare_led(struct sim_lcd_params *slp)
{
	int w, h, x, y, i, j;
	char *p;

	/* Draw simulated flare LED */
	SDL_GetWindowSize(window, &x, &y);
	x = x - 100;
	y = 50;
	draw_led_text(renderer, x, y);
	SDL_SetRenderDrawColor(renderer, led_color.red, led_color.blue, led_color.green, 0xff);
	SDL_RenderFillRect(renderer, &(SDL_Rect) { x, y + 20, 51, 51} );
	SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderDrawRect(renderer, &(SDL_Rect) { x, y + 20, 50, 50} );

	/* If LED is too dim, don't draw it. */
	if (led_color.red < 75 && led_color.green < 75 && led_color.blue < 75)
		return;

	if (slp->orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
		w = badge_image_width;
		h = badge_image_height;
	} else {
		w = landscape_badge_image_width;
		h = landscape_badge_image_height;
	}
	struct button_coord_list bcl = get_button_coords(slp, w, h);
	x = bcl.led.x - led_width / 2;
	y = bcl.led.y - led_height / 2;
	p = led_pixels;
	for (i = 0; i < led_height; i++) {
		for (j = 0; j < led_width; j++) {
			p = &led_pixels[i * led_width * 4 + j * 4];
			p[0] = (char) led_color.red;
			p[1] = (char) led_color.green;
			p[2] = (char) led_color.blue;
		}
	}
	SDL_SetTextureBlendMode(led_image, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(led_image, NULL, led_pixels, led_width * 4);
	SDL_RenderCopy(renderer, led_image, NULL, &(SDL_Rect) { x, y, led_width, led_height});

}

int quit_confirmed(int mousex, int mousey)
{
	int x, y, x1, y1, x2, y2, w, h;

	SDL_GetWindowSize(window, &w, &h);

	x = w / 2 - quit_confirm_width / 2;
	y = h / 2 - quit_confirm_height / 2;

	/* coords of "YES" box */
	x1 = x + 24;
	y1 = y + 72;
	x2 = x + 101;
	y2 = y + 123;

	if (mousex >= x1 && mousex <= x2 && mousey >= y1 && mousey <= y2)
		return 1;
	return 0;
}

static void maybe_draw_quit_confirmation(void)
{
	int x, y, x1, y1;

	SDL_GetWindowSize(window, &x, &y);

	x1 = x / 2 - quit_confirm_width / 2;
	y1 = y / 2 - quit_confirm_height / 2;

	if (!quit_confirm_active)
		return;
	SDL_RenderCopy(renderer, quit_confirm_image, NULL, &(SDL_Rect) { x1, y1, quit_confirm_width, quit_confirm_height });
}

static union quat badge_orientation = IDENTITY_QUAT_INITIALIZER;
static union vec3 gravity_vector = { { 0.0f, 0.0f, 1.0f } };

static union vec3 badge_orientation_points[] = {
	{ { -100.0f, -50.0f, 0.0f } },
	{ { 100.0f, -50.0f, 0.0f } },
	{ { 100.0f, 50.0f, 0.0f } },
	{ { -100.0f, 50.0f, 0.0f } },
	{ { -100.0f, -50.0f, 0.0f } },

	{ { -1, -1, -1 } },

	{ { -50.0f, -40.0f, 0.0f } },
	{ { 50.0f, -40.0f, 0.0f } },
	{ { 50.0f, 45.0f, 0.0f } },
	{ { -50.0f, 45.0f, 0.0f } },
	{ { -50.0f, -40.0f, 0.0f } },

	{ { -1, -1, -1 } },

	{ { -90, -45, 0 } },
	{ { -80, -45, 0 } },
	{ { -80, -35, 0 } },
	{ { -90, -35, 0 } },
	{ { -90, -45, 0 } },

	{ { -1, -1, -1 } },

	{ { 90, -45, 0 } },
	{ { 80, -45, 0 } },
	{ { 80, -35, 0 } },
	{ { 90, -35, 0 } },
	{ { 90, -45, 0 } },

	{ { -1, -1, -1 } },

	{ { 60, 35, 0} },
	{ { 70, 35, 0} },
	{ { 70, 45, 0} },
	{ { 60, 45, 0} },
	{ { 60, 35, 0} },

	{ { -1, -1, -1 } },

	{ { 80, 25, 0} },
	{ { 90, 25, 0} },
	{ { 90, 35, 0} },
	{ { 80, 35, 0} },
	{ { 80, 25, 0} },

	{ { -1, -1, -1 } },

	{ { -80, 20, 0 } },
	{ { -80, 45, 0 } },

	{ { -1, -1, -1 } },

	{ { -90, 33, 0 } },
	{ { -70, 33, 0 } },
};

static union vec3 orientation_indicator_position = { { 0.0f, 0.0f, 100.0f } };

static void draw_badge_orientation_indicator(SDL_Renderer *renderer, float x, float y, float scale, union vec3 *badge_position, union quat *orientation)
{
	const int n = ARRAYSIZE(badge_orientation_points);
	const float camera_z = 100.0f;
	union vec3 indicator[ARRAYSIZE(badge_orientation_points)];

	/* Start with the original 3D orientation indicator coordinates */
	memcpy(indicator, badge_orientation_points, sizeof(indicator));

	/* Rotate the badge into its current 3D orientation */
	for (int i = 0; i < n; i++)
		quat_rot_vec_self(&indicator[i], orientation);

	/* Translate the badge into its current 3D position */
	for (int i = 0; i < n; i++) {
		indicator[i].v.x += badge_position->v.x;
		indicator[i].v.y += badge_position->v.y;
		indicator[i].v.z += badge_position->v.z;
	}

	/* Do perspective transformation */
	for (int i = 0; i < n; i++) {
		indicator[i].v.x = (camera_z * indicator[i].v.x) / (camera_z + indicator[i].v.z);
		indicator[i].v.y = (camera_z * indicator[i].v.y) / (camera_z + indicator[i].v.z);
	}

	/* translate and scale projected (x,y) coords */
	for (int i = 0; i < n; i++) {
		indicator[i].v.x = (scale * indicator[i].v.x) + x;
		indicator[i].v.y = (scale * indicator[i].v.y) + y;
	}

	/* Check if we are seeing the front or the back side of the indicator */
	union vec3 badge_normal = { { 0.0f, 0.0f, -1.0f } };
	union vec3 to_camera = { { 0.0f, 0.0f, -1.0f } };
	quat_rot_vec_self(&badge_normal, orientation);
	float dot = vec3_dot(&badge_normal, &to_camera);

	/* Compute the new gravity vector */
	union quat inverse_rotation;
	gravity_vector.v.x = 0.0f;
	gravity_vector.v.y = 0.0f;
	gravity_vector.v.z = 1.0f;
	quat_inverse(&inverse_rotation, orientation);
	quat_rot_vec_self(&gravity_vector, &inverse_rotation);

	/* Draw the orientation indicator */
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
	SDL_RenderFillRect(renderer, &(SDL_Rect) { 25, 25, 150, 150} );

	if (dot >= 0) /* we see front of badge, blue */
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xff, 0xff);
	else /* we see back of badge, red */
		SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);
	union vec3 *prev = &indicator[0];
	for (int i = 1; i < n; i++) {
		if (fabsf(badge_orientation_points[i].v.z - -1.0f) < 0.001) {
			prev = &indicator[i + 1];
			i++;
			continue;
		}
		float x1 = prev->v.x;
		float y1 = prev->v.y;
		float x2 = indicator[i].v.x;
		float y2 = indicator[i].v.y;
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
		prev = &indicator[i];
	}
}

static int draw_window(SDL_Renderer *renderer, SDL_Texture *texture, SDL_Texture *landscape_texture)
{
    extern uint8_t display_array[LCD_YSIZE][LCD_XSIZE][3];
    struct sim_lcd_params slp = get_sim_lcd_params();

    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderClear(renderer);

    draw_badge_background();
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


    /* Draw a border around the simulated screen */
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderDrawLine(renderer, slp.xoffset - 1, slp.yoffset - 1., slp.xoffset + slp.width + 1, slp.yoffset - 1); /* top */
    SDL_RenderDrawLine(renderer, slp.xoffset - 1, slp.yoffset - 1., slp.xoffset - 1, slp.yoffset + slp.height + 1); /* left */
    SDL_RenderDrawLine(renderer, slp.xoffset - 1, slp.yoffset + slp.height + 1, slp.xoffset + slp.width + 1, slp.yoffset + slp.height + 1); /* bottom */
    SDL_RenderDrawLine(renderer, slp.xoffset + slp.width + 1, slp.yoffset - 1, slp.xoffset + slp.width + 1, slp.yoffset + slp.height + 1); /* right */

    draw_flare_led(&slp);

    draw_badge_orientation_indicator(renderer, 100.0f, 100.0f, 1.0f,
		&orientation_indicator_position, &badge_orientation);

    maybe_draw_quit_confirmation();

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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "Unable to initialize SDL (Video):  %s\n", SDL_GetError());
        return 1;
    }
    if (SDL_NumJoysticks() >= 1)
        joystick = SDL_JoystickOpen(0);
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "Unable to initialize SDL (Events):  %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);
    return 0;
}

static void load_image(char *filename, char **pixels, int *width, int *height)
{
	int w = 0, h = 0, a = 0;
	char whynot[1024];

	if (*pixels)
		return;

	*pixels = png_utils_read_png_image(filename,
		0, 0, 0, &w, &h, &a, whynot, sizeof(whynot) - 1);
	if (!*pixels)
		fprintf(stderr, "Failed to load image \"%s\": %s\n", filename, whynot);
	*width = w;
	*height = h;
}

static void load_badge_images(void)
{

	load_image("../images/badge-image-1024.png", &badge_image_pixels,
			&badge_image_width, &badge_image_height);
	load_image("../images/badge-image-vert-1024.png", &landscape_badge_image_pixels,
			&landscape_badge_image_width, &landscape_badge_image_height);
	load_image("../images/badge-background.png", &badge_background_pixels,
			&badge_background_width, &badge_background_height);
	load_image("../images/badge-led.png", &led_pixels,
			&led_width, &led_height);
	load_image("../images/really-quit.png", &quit_confirm_pixels,
			&quit_confirm_width, &quit_confirm_height);
	printf("quit_confirm_ w,h = %d, %d\n", quit_confirm_width, quit_confirm_height);
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
            mouse_button_down_cb(&event.button, &bcl);
            break;
        case SDL_MOUSEBUTTONUP:
            mouse_button_up_cb(&event.button);
            break;
        case SDL_MOUSEMOTION:
            if (!event.motion.state) /* button held? */
                break;
            /* Check if motion is inside orientation indicator */
            if (event.motion.x < 25)
                break;
            if (event.motion.x > 150)
                break;
            if (event.motion.y < 25)
                break;
            if (event.motion.y > 150)
                break;
            /* We have mouse motion with button held, inside the orientation indicator... */
	    union vec3 u, v;
            u.v.x = 0.0f;
            u.v.y = 0.0f;
            u.v.z = -25.0f;
            v.v.x = event.motion.xrel;
            v.v.y = event.motion.yrel;
            v.v.z = -25.0f;
	    union quat q, new_orientation;
            quat_from_u2v(&q, &u, &v);
	    quat_mul(&new_orientation, &q, &badge_orientation);
            quat_normalize_self(&new_orientation);
            badge_orientation = new_orientation;
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
            mouse_scroll_cb(&event.wheel, &bcl);
            break;
        case SDL_JOYAXISMOTION:
        case SDL_JOYBALLMOTION:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        case SDL_JOYHATMOTION:
            joystick_event_cb(window, event);
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

    init_sim_lcd_params();

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
