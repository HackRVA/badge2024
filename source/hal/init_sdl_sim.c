//
// Created by Samuel Jones on 2/21/22.
//

#include "init.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "framebuffer.h"
#include "display_s6b33.h"
#include "led_pwm.h"
#include "button.h"
#include "button_sdl_ui.h"
#include "ir.h"
#include "rtc.h"
#include "flash_storage.h"

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


// static GtkWidget *vbox, *window, *drawing_area;
#define SCALE_FACTOR 6
#define EXTRA_WIDTH 200
#define GTK_SCREEN_WIDTH (LCD_XSIZE * SCALE_FACTOR + EXTRA_WIDTH)
#define GTK_SCREEN_HEIGHT (LCD_YSIZE * SCALE_FACTOR)
static int real_screen_width = GTK_SCREEN_WIDTH;
static int real_screen_height = GTK_SCREEN_HEIGHT;
// static GdkGC *gc = NULL;               /* our graphics context. */
// static GdkPixbuf *pix_buf;
static int screen_offset_x = 0;
static int screen_offset_y = 0;
static char *program_title;

extern int lcd_brightness;
// extern GdkColor led_color;

// const GdkColor white = {.blue = 65535, .green = 65535, .red = 65535};
// const GdkColor black = {};



static void draw_led_text(/* GtkWidget *widget, */ int x, int y)
{
#if 0
#define LETTER_SPACING 12
    /* Literally draws L E D */
    /* Draw L */
    gdk_draw_line(widget->window, gc, x, y, x, y - 10);
    gdk_draw_line(widget->window, gc, x, y, x + 8, y);

    x += LETTER_SPACING;

    /* Draw E */
    gdk_draw_line(widget->window, gc, x, y, x, y - 10);
    gdk_draw_line(widget->window, gc, x, y, x + 8, y);
    gdk_draw_line(widget->window, gc, x, y - 5, x + 5, y - 5);
    gdk_draw_line(widget->window, gc, x, y - 10, x + 8, y - 10);

    x += LETTER_SPACING;

    /* Draw D */
    gdk_draw_line(widget->window, gc, x, y, x, y - 10);
    gdk_draw_line(widget->window, gc, x, y, x + 8, y);
    gdk_draw_line(widget->window, gc, x, y - 10, x + 8, y - 10);
    gdk_draw_line(widget->window, gc, x + 8, y - 10, x + 10, y - 5);
    gdk_draw_line(widget->window, gc, x + 8, y, x + 10, y - 5);
#endif
}

#if 0
static gint drawing_area_configure(GtkWidget *w, UNUSED GdkEventConfigure *event)
{
    GdkRectangle cliprect;

    /* first time through, gc is null, because gc can't be set without */
    /* a window, but, the window isn't there yet until it's shown, but */
    /* the show generates a configure... chicken and egg.  And we can't */
    /* proceed without gc != NULL...  but, it's ok, because 1st time thru */
    /* we already sort of know the drawing area/window size. */

    if (gc == NULL)
        return TRUE;

    real_screen_width =  w->allocation.width;
    real_screen_height =  w->allocation.height;

    gdk_gc_set_clip_origin(gc, 0, 0);
    cliprect.x = 0;
    cliprect.y = 0;
    cliprect.width = real_screen_width;
    cliprect.height = real_screen_height;
    gdk_gc_set_clip_rectangle(gc, &cliprect);
    return TRUE;
}
#endif

void flareled(unsigned char r, unsigned char g, unsigned char b)
{
    // led_color.red = r * 256;
    // led_color.green = g * 256;
    // led_color.blue = b * 256;
}

#if 0
static void setup_window_geometry(/* GtkWidget *window */)
{
    /* clamp window aspect ratio to constant */
    GdkGeometry geom;
    geom.min_aspect = (gdouble) GTK_SCREEN_WIDTH / (gdouble) GTK_SCREEN_HEIGHT;
    geom.max_aspect = geom.min_aspect;
    gtk_window_set_geometry_hints(GTK_WINDOW(window), NULL, &geom, GDK_HINT_ASPECT);
}

static gboolean delete_event(UNUSED GtkWidget *widget,
                             UNUSED GdkEvent *event, UNUSED gpointer data)
{
    /* If you return FALSE in the "delete_event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete_event". */
    return FALSE;
}

static void destroy(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
    gtk_main_quit();
}

static int draw_window(GtkWidget *widget, UNUSED GdkEvent *event, UNUSED gpointer p) {

    if (time_to_quit) {
        gtk_main_quit();
        return 1;
    }

    cairo_t * cr = gdk_cairo_create(widget->window);
    cairo_scale(cr, SCALE_FACTOR, SCALE_FACTOR);
    gdk_cairo_set_source_pixbuf(cr,pix_buf,0,0);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_paint_with_alpha(cr, (double)lcd_brightness/255.0);
    cairo_fill(cr);
    cairo_destroy(cr);

    int x, y, w, h;
    w = (real_screen_width - EXTRA_WIDTH) / LCD_XSIZE;
    if (w < 1)
        w = 1;
    h = real_screen_height / LCD_YSIZE;
    if (h < 1)
        h = 1;

    /* Draw a vertical line demarcating the right edge of the screen */
    gdk_gc_set_rgb_fg_color(gc, &white);
    gdk_draw_line(widget->window, gc, LCD_XSIZE * w + 1, 0, LCD_XSIZE * w + 1, real_screen_height - 1);

    /* Draw simulated flare LED */
    x = LCD_XSIZE * w + EXTRA_WIDTH / 4;
    y = (LCD_YSIZE * h) / 2 - EXTRA_WIDTH / 4;
    draw_led_text(widget, LCD_XSIZE * w + EXTRA_WIDTH / 2 - 20, y - 10);
    gdk_gc_set_rgb_fg_color(gc, &led_color);
    gdk_draw_rectangle(widget->window, gc, 1 /* filled */, x, y, EXTRA_WIDTH / 2, EXTRA_WIDTH / 2);
    gdk_gc_set_rgb_fg_color(gc, &white);
    gdk_draw_rectangle(widget->window, gc, 0 /* not filled */, x, y, EXTRA_WIDTH / 2, EXTRA_WIDTH / 2);

    return 0;
}

static gboolean draw_window_timer_callback(void* params) {
    GtkWidget *widget = (GtkWidget*)params;
    return (draw_window(widget, NULL, NULL) == 0) ? gtk_true() : gtk_false();
}



static void setup_gtk_window_and_drawing_area(GtkWidget **window, GtkWidget **vbox, GtkWidget **drawing_area)
{
    GdkRectangle cliprect;
    char window_title[1024];

    *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_window_geometry(*window);
    gtk_container_set_border_width(GTK_CONTAINER(*window), 0);
    *vbox = gtk_vbox_new(FALSE, 0);

    extern uint8_t display_array[LCD_YSIZE][LCD_XSIZE][3];

    pix_buf = gdk_pixbuf_new_from_data((const guchar*) display_array, GDK_COLORSPACE_RGB, gtk_false(),
                                        8, 132, 132, 132*3, NULL, NULL);

    gtk_window_move(GTK_WINDOW(*window), screen_offset_x, screen_offset_y);
    *drawing_area = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(*window), "delete_event",
                     G_CALLBACK(delete_event), NULL);
    g_signal_connect(G_OBJECT(*window), "destroy",
                     G_CALLBACK(destroy), NULL);
    g_signal_connect(G_OBJECT(*window), "key_press_event",
                     G_CALLBACK(key_press_cb), "window");
    g_signal_connect(G_OBJECT(*window), "key_release_event",
                     G_CALLBACK(key_release_cb), "window");
    g_signal_connect(G_OBJECT(*drawing_area), "expose_event",
                     G_CALLBACK(draw_window), NULL);
    g_signal_connect(G_OBJECT(*drawing_area), "configure_event",
                     G_CALLBACK(drawing_area_configure), NULL);
    g_timeout_add(16, draw_window_timer_callback, G_OBJECT(*drawing_area));

    gtk_container_add(GTK_CONTAINER(*window), *vbox);
    gtk_box_pack_start(GTK_BOX(*vbox), *drawing_area, TRUE /* expand */, TRUE /* fill */, 0);
    gtk_window_set_default_size(GTK_WINDOW(*window), real_screen_width, real_screen_height);
    snprintf(window_title, sizeof(window_title), "HackRVA Badge Emulator - %s", program_title);
    free(program_title);
    gtk_window_set_title(GTK_WINDOW(*window), window_title);


    gtk_widget_modify_bg(*drawing_area, GTK_STATE_NORMAL, &black);
    gtk_widget_show(*vbox);
    gtk_widget_show(*drawing_area);
    gtk_widget_show(*window);
    gc = gdk_gc_new(GTK_WIDGET(*drawing_area)->window);

    gdk_gc_set_rgb_fg_color(gc, &white);

    gdk_gc_set_clip_origin(gc, 0, 0);
    cliprect.x = 0;
    cliprect.y = 0;
    cliprect.width = real_screen_width;
    cliprect.height = real_screen_height;
    gdk_gc_set_clip_rectangle(gc, &cliprect);
}
#endif


void hal_start_sdl(int *argc, char ***argv) {
    program_title = strdup((*argv)[0]);
#if 0
    gtk_set_locale();
    gtk_init(argc, argv);
    setup_gtk_window_and_drawing_area(&window, &vbox, &drawing_area);
#endif
    flareled(0, 0, 0);

}
