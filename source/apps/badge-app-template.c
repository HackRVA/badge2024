/*********************************************

This is a badge app template.

To use this file, do the following steps:

1. Copy the file to a different name, the name of your app.
   e.g. cp badge-app-template.c myapp.c

   (Don't call it "myapp" please.)

2. Copy ./badge-app-template.h to ./myapp.h

   (Don't call it "myapp" please.)

2. Edit the file.  Change myprogram_cb to some other name, e.g. "myapp_cb"
   wherever it appears in this file. (Think of a real name, don't actually
   use myapp_cb, please.)

3. Add your app to the CMakeLists.txt file in this directory, for example:

CMakeLists.txt:
...
        ${CMAKE_CURRENT_LIST_DIR}/smashout.c
        ${CMAKE_CURRENT_LIST_DIR}/spacetripper.c
        ${CMAKE_CURRENT_LIST_DIR}/username.c
        ${CMAKE_CURRENT_LIST_DIR}/myapp.c      # Add my app!
        )
...

4.  In source/core/menu.c:

    - Include your new app header.
    - Add a new entry in the `games_m` menu structure for your app.
    - Optional: To start the simulator running your app (skipping the menus), change the runningApp variable to your
      app callback.

    For example:

menu.c:
...
//Apps
...
#include "myapp.h"
...

...
void (*runningApp)() = app_cb; // Don't commit this change; just for local testing
...

...
const struct menu_t games_m[] = {
   {"Blinkenlights", VERT_ITEM|DEFAULT_ITEM, FUNCTION, {(struct menu_t *)blinkenlights_cb}},
   ...
   {"My App",    VERT_ITEM, FUNCTION, {(struct menu_t *)app_cb},
   {"Back",	     VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};
...

5.  Build the linux program, from the top level of the repository

    # (you only need to run cmake after modifying a CMakeLists.txt file)
    cmake -S . -B build_sim/ -DCMAKE_BUILD_TYPE=Debug -DTARGET=SIMULATOR -G "Unix Makefiles"

    cd build_sim
    make

6.  Delete all these instruction comments from your copy of the file.

7.  Modify the program to make it do what you want.

**********************************************/

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"


/* Program states.  Initial state is MYPROGRAM_INIT */
enum myprogram_state_t {
	MYPROGRAM_INIT,
	MYPROGRAM_RUN,
	MYPROGRAM_EXIT,
};

static enum myprogram_state_t myprogram_state = MYPROGRAM_INIT;
static int screen_changed = 0;

static void myprogram_init(void)
{
	FbInit();
	FbClear();
	myprogram_state = MYPROGRAM_RUN;
	screen_changed = 1;
}

static void check_buttons()
{
    int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches)) {
		/* Pressing the button exits the program. You probably want to change this. */
		myprogram_state = MYPROGRAM_EXIT;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
	}
}

static void draw_screen()
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	FbMove(10, LCD_YSIZE / 2);
	FbWriteLine("HOWDY!");
	FbSwapBuffers();
	screen_changed = 0;
}

static void myprogram_run()
{
	check_buttons();
	draw_screen();
}

static void myprogram_exit()
{
	myprogram_state = MYPROGRAM_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

/* You will need to rename myprogram_cb() something else. */
void myprogram_cb(void)
{
	switch (myprogram_state) {
	case MYPROGRAM_INIT:
		myprogram_init();
		break;
	case MYPROGRAM_RUN:
		myprogram_run();
		break;
	case MYPROGRAM_EXIT:
		myprogram_exit();
		break;
	default:
		break;
	}
}

