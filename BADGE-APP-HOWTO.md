
BADGE APP HOWTO
---------------

This assumes you've got your system set up and are able to compile
the badge firmware and the badge simulator, and that now you're ready
to start working on your badge app.

It is also assumed that you are an OK C programmer, though even beginner
C programmers are encouraged to give it a try.

Overview
--------

The badge uses a simple, cooperative multitasking system with no memory
protection. Each badge app must provide a callback function (that you write),
and when the badge app is active, that function will be repeatedly called.

This function can do whatever you want it to, but it must return
within a short time as part of the "cooperative multitasking". If it
doesn't return in a timely manner, certain other functions of the badge
may not work correctly (e.g. the clock might not advance correctly, LEDs
might not blink, USB serial traffic may not work, etc.)

When the badge app is ready to exit, it should call returnToMenus().

A typical badge app callback function might look something like this:


```c
static enum my_badge_app_state_t {
	MY_BADGE_APP_INIT = 0,
	MY_BADGE_APP_POLL_INPUT,
	MY_BADGE_APP_DRAW_SCREEN,
	MY_BADGE_APP_EXIT,
	/* ... whatever other states your badge app might have go here ... */
} my_badge_app_state = MY_BADGE_APP_INIT;

int my_badge_app_cb(void)
{
	switch (my_badge_app_state) {
	case MY_BADGE_APP_INIT:
		init_badge_app();	/* you write this function */
		break;
	MY_BADGE_APP_POLL_INPUT:
		poll_input();		/* you write this function */
		break;
	MY_BADGE_APP_DRAW_SCREEN:
		draw_screen();		/* you write this function */
		break;
	MY_BADGE_APP_EXIT:
		returnToMenus();
		break;

	/* ... Etc ... */

	default:
		break;
	}
	return 0;
}
```

Badge App Template
------------------

To get started quickly, a badge app template file is provided which you can copy
and modify to get started making your badge app quickly without having to type in
a bunch of boilerplate code before anything even begins to work.

* [source/apps/badge-app-template.c](https://github.com/HackRVA/badge2023/blob/main/source/apps/badge-app-template.c)

It is extensively commented to help you know how it's meant to be used.

Drawing on the Screen
---------------------

The screen is 128x160 pixels and the interface for drawing on the screen
is mostly defined in [source/display/frambuffer.h](https://github.com/HackRVA/badge2023/blob/main/source/display/framebuffer.h).
(BTW, use the macros `LCD_XSIZE` and `LCD_YSIZE` rather than hard coding screen dimensions.)

Here is a (probably incomplete) list of functions you may call for drawing on the screen:

Initializing and Clearing the Framebuffer
-----------------------------------------

```
	void FbInit(void);
		You must call this before doing anything else with the framebuffer

	void FbClear(void);
		Clear the framebuffer with the background color
```

Updating the Screen
-------------------

```
	void FbSwapBuffers();
		Copy the framebuffer contents to the actual screen. This makes whatever
		changes you've made to the framebuffer visible on the badge screen. This
		call is quite slow, but you ought to be able to attain 30 frames per second.

	void FbPushBuffer(void);
		Similar to FbSwapBuffers(), but it does not clear the physical screen first.
		This can be useful for making incremental changes to a consistent scene.
		(NOTE: the linux emulator does not currently implement this correctly.)
```

Colors
------

There are a few named colors defined in
[source/display/colors.h](https://github.com/HackRVA/badge2023/blob/main/source/display/colors.h)
and a lot more named colors defined in
[source/display/x11_colors.h](https://github.com/HackRVA/badge2023/blob/main/source/display/x11_colors.h)


```
	void FbColor(int color);
		Sets the current foreground color, which subsequent drawing functions will then use.

	void FbBackgroundColor(int color);
		Sets the current background color, which subsequent drawing functions and FbClear()
		will then use.
```

Example usage:

```
	FbColor(x11_goldenrod);
	FbBackgroundColor(x11_firebrick1);
```

You can construct your own custom colors from RGB values.  There are 5 bits for red, 6 bits for green,
and 5 bits for blue.  For example:

```
	unsigned char red = r; /* whatever arbitrary values */
	unsigned char green = g;
	unsigned char blue = b;

	unsigned short random_color = ((red & 0xf8) << 8) | ((green & 0xfc) << 3) | ((blue & 0xf8) >> 3);
	FbColor(random_color);
```

Drawing Lines and Points and other things
-----------------------------------------

Note many functions take coordinates as type "unsigned char".  Beware of passing larger
integer types (e.g. "int") as these will be wrapped into the range 0-255.


```
	void FbPoint(unsigned char x, unsigned char y);
		plots a point on the framebuffer at (x, y)

	void FbLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
		draws a line from (x1, y1) to (x2, y2)

	void FbHorizontalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
		draws a line from (x1, y1) to (x2, y1) (y2 is unused)  Faster than FbLine().

	void FbVerticalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
		draws a line from (x1, y1) to (x1, y2) (x2 is unused)  Faster than FbLine().

	void FbFilledRectangle(unsigned char width, unsigned char height);
	void FbRectangle(unsigned char width, unsigned char height);
		draws filled or unfilled rectangles at the current cursor postion.
		The cursor is moved to (x + width, y + height).

	void FbCircle(int x, int y, int r);
		draws a circle centered at (x, y) with radius r.

	struct point
	{
		signed char x, y;
	};

	void FbDrawObject(const struct point drawing[], int npoints, int color, int x, int y, int scale);
		FbDrawObject() draws an object at x, y.  The coordinates of drawing[] should be centered at
		(0, 0).  The coordinates in drawing[] are multiplied by scale, then divided by 1024 (via a shift)
		so for 1:1 size, use scale of 1024.  Smaller values will scale the object down. This is different
		than FbPolygonFromPoints() or FbDrawVectors() in that drawing[] contains signed chars, and the
		polygons can be constructed via this program: https://github.com/smcameron/vectordraw
		as well as allowing scaling.
```

The following are also available: however they are not well documented.

```
	void FbImage(unsigned char assetId, unsigned char seqNum);
	void FbImage8bit(unsigned char assetId, unsigned char seqNum);
	void FbImage4bit(unsigned char assetId, unsigned char seqNum);
	void FbImage2bit(unsigned char assetId, unsigned char seqNum);
	void FbImage1bit(unsigned char assetId, unsigned char seqNum

```

Moving the "Cursor"
-------------------

A "graphics cursor" is maintained for the framebuffer.  This amounts
to a "current (x,y) position" within the framebuffer that is implicitly
used by various graphics drawing functions to indicate where on the
framebuffer things should be drawn. There are a number of functions to
adjust the position indicated by the cursor, described below.

```
	void FbMove(unsigned char x, unsigned char y);
		Move the cursor to (x, y)

	void FbMoveRelative(char x, char y);
		Move the cursor relative to its current position, you can think of it as doing:

			cursor.x += x;
			cursor.y += y;

	void FbMoveX(unsigned char x);
		Sets the x position of the cursor

	void FbMoveY(unsigned char y);
		Sets the y position of the cursor
```

Writing Strings to the Framebuffer
----------------------------------

```
	All strings are assumed to be NULL terminated.

	void FbWriteLine(char *s);
		Writes the string s to the framebuffer. The cursor is positioned
		just to the right of the last character of the string. Newlines are
		not handled specially.

	void FbWriteString(char *s);
		Writes a string with the specified length to the framebuffer.
		Newlines are handled.
```

Buttons, Directional-Pad Inputs and Rotary Encoders
---------------------------------------------------

The API for badge-apps to deal with buttons and rotary encoders is defined
in [source/hal/button.h](https://github.com/HackRVA/badge2023/blob/main/source/hal/button.h)

There is an enumerated type defining a value for each button:
```

	typedef enum {
    		BADGE_BUTTON_LEFT = 0,
    		BADGE_BUTTON_DOWN,
    		BADGE_BUTTON_UP,
    		BADGE_BUTTON_RIGHT,
    		BADGE_BUTTON_SW,
    		BADGE_BUTTON_SW2,
    		BADGE_BUTTON_ENCODER_A,
    		BADGE_BUTTON_ENCODER_B,
    		BADGE_BUTTON_MAX
	} BADGE_BUTTON;
```

There are three functions for getting the current state of the buttons
all at once.

```
	int button_down_latches(void);
	int button_up_latches(void);
	void clear_latches(void);
```

and a convenience macro for interrogating the result returned by button_down_latches() or button_up_latches():

```
	#define BUTTON_PRESSED(button, latches) ((latches) & (1 << (button)))
```

Typical usage would be:

```
	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches))
		do_whatever_when_up_button_is_pressed();
```

Note that with this system, if you press the button once and hold it, button_down_latches()
will return only one button press until the user releases the button and presses it again.
This is not always what you want.  Sometimes you want to allow the user to press and hold
the button, and register the button as being active for as long as they hold it down. For this,
there is button_poll() to get the current state of a particular button.

```
	int button_poll(BADGE_BUTTON button);
```

If you want the current state of all the buttons at once, you can use button_mask().

```
	int button_mask();
```

You can get a timestamp of the last button press, or reset the timestamp
of the last button pressed. I believe these are used to implement the
screensaver feature.

```
	unsigned int button_last_input_timestamp(void);
	void button_reset_last_input_timestamp(void);
```

There are two rotary encoders, 0 and 1, which can be queried:

```
	// Get rotary encoder rotations! Rotation count is automatically cleared between calls.
	// Positive indicates CW, negative indicates CCW.
	int button_get_rotation(int which_rotary);
```

Display Related Functions
-------------------------

These functions are related to the hardware display as opposed to the
framebuffer functions described earlier that are used for normal drawing.

See: [source/hal/display.h](https://github.com/HackRVA/badge2023/blob/main/source/hal/display.h)

The display brightness can be controlled via led_pwd_enable(BADGE_LED_DISPLAY_BACKLIGHT, duty);
See: [source/hal/led_pwm.h](https://github.com/HackRVA/badge2023/blob/main/source/hal/led_pwm.h)

Infrared Transmitter and Receiver
---------------------------------

```
		TODO: This documentation is not very good.

		Look in badge_monsters.c and lasertag.c badge apps to see examples of the IR
		system being used. The badge_monsters.c one is simpler, the lasertag.c one is
		more complicated.

		/* Emulate some stuff from ir.c */
		extern int IRpacketOutNext;
		extern int IRpacketOutCurr;
		#define MAXPACKETQUEUE 16
		#define IR_OUTPUT_QUEUE_FULL (((IRpacketOutNext+1) % MAXPACKETQUEUE) == IRpacketOutCurr)

		struct IRpacket_t {
			/* Note: this is different than what the badge code actually uses.
			 * The badge code uses 32-bits worth of bitfields.  Bitfields are
			 * not portable, nor are they clear, and *which* particular bits
			 * any bitfields land in are completely up to the compiler.
			 * Relying on the compiler using any particular bits is a programming
			 * error, so don't actually use this. Instead always
			 * use the "v" element of the IRpacket_u union.
			 */
			unsigned int v;
		};

		union IRpacket_u {
			struct IRpacket_t p;		/* <---- DON'T USE THIS */
			unsigned int v;			/* <---- USE THIS */
		};


		void IRqueueSend(union IRpacket_u pkt);
			Queues an IR packet for transmission.

		/* This is tentative.  Not really sure how the IR stuff works.
		   For now, I will assume I register a callback to receive 32-bit
		   packets incoming from IR sensor. */
		void register_ir_packet_callback(void (*callback)(struct IRpacket_t));
		void unregister_ir_packet_callback(void);

		#define IR_APP1 19
		#define IR_APP2 20
		#define IR_APP3 21
		#define IR_APP4 22
		#define IR_APP5 23
		#define IR_APP6 24
		#define IR_APP7 25
```

Badge ID and User Name:
-----------------------

```
	Each badge has a unique ID which is a 16-bit unsigned short burned into the firmware
	at the time the badge is firmware is flashed, and a user assignable name (using the
	username badge app).

		extern struct sysData_t {
			char name[16];
			unsigned short badgeId;
			/* TODO: There is more stuff in the badge which we might want to expose if
			 * an app needs it.
			 */
		} G_sysData;
```

Audio
-----

TODO: Elaborate on this

See: [source/hal/audio.h](https://github.com/HackRVA/badge2023/blob/main/source/hal/audio.h)

Note: The badge simulator does not currently provide any audio functionality

Setting the Flair LED Color
---------------------------

This year's badge has no flair LED.  Instead we have an extra rotary encoder and button.

Flash Memory Access
-------------------

For lower level access to flash API:
see: [source/hal/flash_storage.h](https://github.com/HackRVA/badge2023/blob/main/source/hal/flash_storage.h)

For a higher level flash-backed key-value store API:
see: [source/core/key_value_storage.h](https://github.com/HackRVA/badge2023/blob/main/source/core/key_value_storage.h)

Exiting the Badge App:
----------------------

```
	void returnToMenus(void);
		"Exits" the badge app and returns to the main menu.
```

Miscellaneous Functions:
------------------------

```

	void itoa(char *string, int value, int base);
		Converts the integer value into an ascii string in the given base. The string
		must have enough memory to hold the converted value.  (Note: the linux badge
		emulator currently ignores base, and assumes you meant base 10.)

	char *strcpy(char *dest, const char *src);
		Copies NULL terminated string src to dest. There must be enough room in dest.

	char *strcat(char *dest, const char *src);
		Concatenates NULL terminated string src onto the end of NULL terminated string dest.
		There must be enough room in dest.

	int abs(int x);
		Returns the absolute value of integer x.

	short sine(int a);
	short cosine(int a);
		Returns 1024 * the sine and 1024 * the cosine of angle a, where a is in units
		of 128th's of a circle. Typical usage is as follows:

		int angle_in_degress = 90; /* degrees */
		int speed = 20;

		int vx = (speed * cosine((128 * angle_in_degrees) / 360)) / 1024;
		int vy = (speed * -sine((128 * angle_in_degrees) / 360)) / 1024;

	int arctan2(int y, int x);
		The angles returned by arctan2 are in the range -64 to +64, corresponding
		to -180 and +180 degrees.

	int fxp_sqrt(int x);
		Fixed point square root.
		See: [source/core/fxp_sqrt.h](https://github.com/HackRVA/badge2023/blob/main/source/core/fxp_sqrt.h)


```

Pseudorandom numbers
--------------------

There is a hardware based random number generator defined in
[source/hal/random.h](https://github.com/HackRVA/badge2023/blob/main/source/hal/random.h)

```
	void random_insecure_bytes(uint8_t *bytes, size_t len);
```

Note: the badge simulator does not have a hardware based random number
generator.

And there is a pseudo-random number generator defined in random.h as well:

```
	uint32_t random_insecure_u32_congruence(uint32_t last);
```

There is another pseudorandom number generator defined in
[source/core/xorshift.h](https://github.com/HackRVA/badge2023/blob/main/source/core/xorshift.h)


```
	unsigned int xorshift(unsigned int *state);
		Returns a pseudo random number derived from seed, *state, and scrambles
		*state to prepare for the next call.  The state variable should be initialized
		to a random seed which should not be zero, and should contain a fair number of
		1 bits and a fair number of 0 bits.
```

Typical usage:

```

	unsigned int my_state = 0xa5a5a5a5 ^ timestamp; // For example.


	int random_number_between_0_and_1000(void)
	{
		return (xorshift(&my_state) % 1000);
	}

```

For more information about how this pseudorandom number generator is implemented,
see https://en.wikipedia.org/wiki/Xorshift#Example_implementation.


Menus
-----

There is a library for badge app menus. The interface for this library is defined in
[source/core/dynmenu.h](https://github.com/HackRVA/badge2023/blob/master/source/core/dynmenu.h)
and the implementation is in [source/core/dynmenu.c](https://github.com/HackRVA/badge2020/blob/master/source/core/dynmenu.c).
The types and functions declared in dynmenu.h and the way they are meant to be used are are
fairly well documented within [the header file](https://github.com/HackRVA/badge2020/blob/master/include/dynmenu.h).

There is also *another*, different menu system defined in
[source/core/menu.h](https://github.com/HackRVA/badge2020/blob/master/source/core/menu.h),
which is used for the badge main menu, and it can also be used by badge apps.
The API for this is a bit more complicated, while at the same time more
limiting, at least in some ways, as it does not (I think) allow for dynamically
changing the menu elements. That is, if you need to have a menu item like "Take X",
where X is replaced at runtime with some other value not known at compile time,
then core/menu.h won't help you (source/apps/maze.c has menu items like this).

Pathfinding
-----------

Many games may need to do pathfinding, so an implementation of
the [A-Star algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm) is provided
via [source/core/a_star.h](https://github.com/HackRVA/badge2023/blob/main/source/core/a_star.h)

A simple example demonstrating the use of this a-star implementation is provided in
[source/core/test_a_star.c](https://github.com/HackRVA/badge2023/blob/main/source/core/test_a_star.c)

Ray Casting
-----------

Games frequently have a need to perform some ray-casting operations using Bresenham's line
drawing algorithm.  For this the bline() function is provided via
[source/core/bline.h](https://github.com/HackRVA/badge2023/blob/main/source/core/bline.h)

```
	typedef int (*plotting_function)(int x, int y, void *context);

	extern void bline(int x1, int y1, int x2, int y2, plotting_function plot_func, void *context);
```

The user-provided plot_function will be called with the context pointer for every point that
Bresenham's algorithm visits.  An example using bline() can be found in
[source/apps/gulag.c](https://github.com/HackRVA/badge2023/blob/main/source/apps/gulag.c#L2499)
where it is used to do collision detection between bullets and walls.


