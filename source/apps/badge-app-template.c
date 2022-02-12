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

3. Modify linux/Makefile to build your program, for example:

    TODO SAM UPDATE FOR CMAKE

4. Modify the main Makefile, for example:

    TODO SAM UPDATE FOR CMAKE

6.  Build the linux program:

    cd linux
    make bin/myapp

    Or whatever you called it, (you didn't call it myapp, right?)

7.  Build the main program:

    cd ..
    make clean
    make

8.  Delete all these instruction comments from your copy of the file.

9.  Modify the program to make it do what you want.

**********************************************/
#ifdef __linux__
#include <stdio.h>
#include <sys/time.h> /* for gettimeofday */
#include <string.h> /* for memset */

#include "../linux/linuxcompat.h"
#include "../linux/bline.h"
#else
#include <string.h>
#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#endif


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
	if (down_latches & (1<<BADGE_BUTTON_SW)) {
		/* Pressing the button exits the program. You probably want to change this. */
		myprogram_state = MYPROGRAM_EXIT;
	} else if (down_latches & (1<<BADGE_BUTTON_LEFT)) {
	} else if (down_latches & (1<<BADGE_BUTTON_RIGHT)) {
	} else if (down_latches & (1<<BADGE_BUTTON_UP)) {
	} else if (down_latches & (1<<BADGE_BUTTON_DOWN)) {
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
int myprogram_cb(void)
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
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, myprogram_cb, 30);
        return 0;
}
#endif
