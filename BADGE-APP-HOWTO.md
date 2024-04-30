
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

* [source/apps/badge-app-template.c](https://github.com/HackRVA/badge2024/blob/main/source/apps/badge-app-template.c)

It is extensively commented to help you know how it's meant to be used.

Drawing on the Screen
---------------------

The screen is 128x160 pixels and the interface for drawing on the screen
is mostly defined in [source/display/frambuffer.h](https://github.com/HackRVA/badge2024/blob/main/source/display/framebuffer.h).
(BTW, use the macros `LCD_XSIZE` and `LCD_YSIZE` rather than hard coding screen dimensions.)

Here is a (probably incomplete) list of functions you may call for drawing on the screen:

Initializing and Clearing the Framebuffer
-----------------------------------------

```
	#include "framebuffer.h"

	void FbInit(void);
		You must call this before doing anything else with the framebuffer

	void FbClear(void);
		Clear the framebuffer with the background color
```

Updating the Screen
-------------------

```
	#include "framebuffer.h"

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
[source/display/colors.h](https://github.com/HackRVA/badge2024/blob/main/source/display/colors.h)
and a lot more named colors defined in
[source/display/x11_colors.h](https://github.com/HackRVA/badge2024/blob/main/source/display/x11_colors.h)


```
	#include "framebuffer.h"
	#include "colors.h"

	void FbColor(int color);
		Sets the current foreground color, which subsequent drawing functions will then use.

	void FbBackgroundColor(int color);
		Sets the current background color, which subsequent drawing functions and FbClear()
		will then use.
```

Example usage:

```
	#include "framebuffer.h"
	#include "colors.h"

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
	#include "framebuffer.h"

	void FbPoint(unsigned char x, unsigned char y);
		plots a point on the framebuffer at (x, y)

	void FbLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
		draws a line from (x1, y1) to (x2, y2)

	void FbClippedLine(signed short x1, signed short y1, signed short x2, signed short y2);
		draws a line from (x1, y1) to (x2, y2), clipped to the screen dimensions.  At least one
		of (x1, y1), (x2, y2) must be on screen.  This function does a per-pixel test, so is slightly
		slower than FbLine().

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

	int FbOnScreen(int x, int y);
		returns true if x, y is on the display, false otherwise.
		(0 <= x < LCD_XSIZE and 0 <= y < LCD_YSIZE)

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

The following are also available for displaying images or "sprites".

```
	#include "framebuffer.h"

	void FbImage2(const struct asset2 *asset, unsigned char seqNum);
	void FbImage16bit2(const struct asset2 *asset, unsigned char seqNum);
	void FbImage8bit2(const struct asset2 *asset, unsigned char seqNum);
	void FbImage4bit2(const struct asset2 *asset, unsigned char seqNum);
	void FbImage2bit2(const struct asset2 *asset, unsigned char seqNum);
	void FbImage1bit2(const struct asset2 *asset, unsigned char seqNum
```

The upper left corner of the image will be drawn at the current location of
the graphics cursor (see FbMove().)

FbImage2(), FbImage8bit2(), etc. replace the older FbImage(), and FbImage8bit2, etc.
(at some point when all uses of the old functions are converted to the new
functions, the original functions will be removed, and the new functions renamed
without the "2").

There are 5 different image formats, with 16, 8, 4, 2, and 1 bits per pixel.
FbImage2() will automatically dispatch to one of the format specific functions,
but if you know the format you can call the format specific functions directly.

The 8, 4, and 2 bit formats use a colormap, and support 255, 3 and 1 colors
respectively, plus "transparent." They way it works is that a number of 16 bit
representations of color values (5 bits each for R, G, and B) is stored in
an array of colors, and then for each pixel, an index into this array is stored.
An index with all bits set represents transparent.

The 1 and 16 bit image formats do not use a colormap, and store the color
information directly, using 1 bit or 16 bits per pixel respectively. For the
1 bit images, the current foreground and background colors are used (see
FbColor() and FbBackgroundColor()).  Neither the 1 or 16 bit formats support
transparency.

There is a program, tools/png-to-badge-asset.c which will convert a PNG file
to C code for any of the 5 image formats. You might need to posterize your
images to ensure they have few enough colors for whichever image format you
want.

For example, here is the output for a 16x16 image, star.png into the 8 bit format:

```
$ ../../tools/png-to-badge-asset 8 badgey_assets/star.png
Processing badgey_assets/star.png
/* Begin code generated by png-to-badge-asset from badgey_assets/star.png Sat Apr 13 08:29:05 2024. */
static const uint16_t star_colormap[51] = {
	0x1081, 0x0000, 0xa4cd, 0x5286, 0x3143, 0x6b08, 0x2902, 0x0800, 0x39c4, 0x3184, 0x3984, 0x18c1,
	0x4204, 0x39c5, 0x5246, 0x62c7, 0x5ac7, 0x5a86, 0x7348, 0xa4cc, 0xa48c, 0x83c9, 0x2943, 0xc5ce,
	0xe690, 0xeed0, 0x83ca, 0x9c4b, 0xc58e, 0xde90, 0xe6d0, 0xf757, 0x944b, 0xeed1, 0xf711, 0xff57,
	0xad0e, 0x944c, 0x2102, 0x4a46, 0xde4f, 0x1881, 0x940a, 0x7b89, 0x5a87, 0x41c4, 0x6b07, 0x2903,
	0x4a05, 0x2942, 0x0840,
}; /* 51 values */

static const uint8_t star_data[256] = {
	0, 0, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1,
	1, 1, 1, 2, 5, 1, 1, 1, 1, 1, 6, 7, 1, 0, 8, 1, 1, 1, 1, 9,
	2, 1, 1, 1, 1, 10, 11, 1, 1, 1, 12, 13, 1, 1, 1, 3, 2, 1, 1, 1,
	14, 8, 1, 1, 1, 1, 1, 15, 16, 1, 1, 2, 2, 1, 2, 2, 17, 1, 1, 1,
	1, 1, 1, 2, 2, 18, 2, 2, 19, 2, 20, 21, 1, 1, 1, 1, 1, 1, 1, 1,
	22, 23, 2, 24, 25, 23, 23, 1, 1, 1, 1, 1, 26, 2, 2, 27, 28, 29, 30, 31,
	31, 23, 23, 2, 2, 32, 2, 2, 2, 23, 2, 23, 23, 33, 34, 35, 31, 24, 2, 36,
	37, 2, 2, 5, 1, 38, 39, 2, 2, 2, 29, 25, 25, 40, 2, 17, 41, 10, 11, 1,
	1, 1, 1, 3, 3, 23, 2, 23, 40, 23, 23, 2, 1, 1, 1, 1, 1, 1, 1, 9,
	2, 16, 2, 2, 23, 42, 19, 5, 1, 1, 1, 1, 1, 1, 8, 43, 2, 44, 45, 2,
	2, 10, 8, 46, 1, 1, 1, 1, 1, 47, 48, 1, 1, 1, 1, 2, 39, 1, 1, 45,
	8, 1, 1, 1, 49, 4, 1, 1, 1, 1, 1, 2, 16, 1, 1, 1, 1, 1, 1, 1,
	50, 1, 1, 1, 1, 1, 1, 15, 2, 1, 1, 1, 1, 1, 1, 1,
}; /* 256 values */

static const struct asset2 star = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) star_colormap,
	.pixel = (const unsigned char *) star_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/star.png Sat Apr 13 08:29:05 2024 */
Processed 1 file(s) with 0 error(s) and 0 warning(s).
```

Note that the diagnostic messages are output to stderr, while only the code goes to stdout,
so you could redirect the output to a file, or you can just grab the code with the mouse
and paste it into your program.

Note that the "transparency" supported by the 8, 4, and 2 bit image formats is not
quite equivalent to the alpha channel supported by PNG files, which support partial
transparency (tinting, or translucency).  For the badge image formats, pixels are either
completely opaque, or completely transparent.  If the alpha channel value for a pixel
in the decoded PNG data is less than 128, then it will be rendered transparent, otherwise
opaque by png-to-badge-asset.

For the original image format used by FbImage, there is an older
python program that converts images, if you can get it to work.
Take a look at [tools/README.md](tools/README.md)
which explains how to convert images to a form usable by the original
FbImage functions the badge software.
Look at source/apps/clue.c and source/apps/clue_assets/ to see an example
of a badge app that uses the images produced by the process described
int tools/README.md

Moving the "Cursor"
-------------------

A "graphics cursor" is maintained for the framebuffer.  This amounts
to a "current (x,y) position" within the framebuffer that is implicitly
used by various graphics drawing functions to indicate where on the
framebuffer things should be drawn. There are a number of functions to
adjust the position indicated by the cursor, described below.

```
	#include "framebuffer.h"

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
		Writes a string to the framebuffer.
		Newlines are handled.

	void FbRotWriteString(char *s);
		Writes a string rotated 90 degrees clockwise.
		Newlines are handled.
```

Buttons, Directional-Pad Inputs and Rotary Encoders
---------------------------------------------------

NOTE: The 2024 badge does not have rotary encoders, just the D-pad
and A and B buttons.

The API for badge-apps to deal with buttons and rotary encoders is defined
in [source/hal/button.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/button.h)

There is an enumerated type defining a value for each button in source/hal/button.h:
```

	typedef enum {
		BADGE_BUTTON_A,			/* The "A" button */
		BADGE_BUTTON_B,			/* The "B" button */
		BADGE_BUTTON_LEFT = 0,		/* Left dpad button */
		BADGE_BUTTON_DOWN,		/* Down dpad button */
		BADGE_BUTTON_UP,		/* Up dpad button */
		BADGE_BUTTON_RIGHT,		/* Right dpad button */
        #if BADGE_HAS_ROTARY_ENCODERS
		BADGE_BUTTON_ENCODER_SW,	/* Push button on the Right rotary encoder */
		BADGE_BUTTON_ENCODER_A,		/* You probably don't want to use this one */
		BADGE_BUTTON_ENCODER_B,		/* You probably don't want to use this one */
		BADGE_BUTTON_ENCODER_2_SW,	/* Push button on the Left rotary encoder */
		BADGE_BUTTON_ENCODER_2_A,	/* You probably don't want to use this one */
		BADGE_BUTTON_ENCODER_2_B,	/* You probably don't want to use this one */
        #endif
		BADGE_BUTTON_MAX		/* sentinal value, does not correspond to a button */
	} BADGE_BUTTON;
```

There are three functions for getting and clearing the current state of the buttons
all at once.

```
	#include "button.h"

	int button_down_latches(void);
	int button_up_latches(void);
	void clear_latches(void);
```

and a convenience macro for interrogating the result returned by button_down_latches() or button_up_latches():

```
	#include "button.h"

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
	#include "button.h"

	int button_poll(BADGE_BUTTON button);
```

If you want the current state of all the buttons at once, you can use button_mask().

```
	#include "button.h"

	int button_mask();
```

You can get a timestamp of the last button press, or reset the timestamp
of the last button pressed. You can periodically call
button_reset_last_input_timestamp() to suppress the screensaver, if you need to.

```
	#include "button.h"

	unsigned int button_last_input_timestamp(void);
	void button_reset_last_input_timestamp(void);
```
NOTE: the 2024 badge does NOT have rotary encoders.

There are two rotary encoders, 0 and 1, which can be queried:
(For 2024, button\_get\_rotation() exists, but it always returns 0.)

```
	#include "button.h"

	// Get rotary encoder rotations! Rotation count is automatically cleared between calls.
	// Positive indicates CW, negative indicates CCW.
	int button_get_rotation(int which_rotary); // always returns 0 in 2024 badge.
```

The four enumeration values associated with the rotary encoders
(These enumeration values do not exist in the 2024 badge.)

```
	#include "button.h"

	BADGE_BUTTON_ENCODER_A,		/* You probably don't want to use this one */
	BADGE_BUTTON_ENCODER_B,		/* You probably don't want to use this one */
	BADGE_BUTTON_ENCODER_2_A,	/* You probably don't want to use this one */
	BADGE_BUTTON_ENCODER_2_B,	/* You probably don't want to use this one */
```

have to do with how the rotation information is encoded.  You should almost certainly not
use these enumeration values directly but instead get the rotation information by calling
button\_get\_rotation();

```
	#include "button.h"

	int rot1 = button_get_rotation(0); /* get rotation info for the encoder on the right of the badge */
	int rot2 = button_get_rotation(1); /* get rotation info for the encoder on the left of the badge */
```

Display Related Functions
-------------------------

These functions are related to the hardware display as opposed to the
framebuffer functions described earlier that are used for normal drawing.

See: [source/hal/display.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/display.h)

The display brightness can be controlled via led_pwd_enable(BADGE_LED_DISPLAY_BACKLIGHT, duty);
See: [source/hal/led_pwm.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/led_pwm.h)

Infrared Transmitter and Receiver
---------------------------------

## Transmitting IR

Transmitting IR is fairly simple. There is an IR_DATA type defined in source/hal/ir.h.

```
	typedef struct {
	    uint16_t recipient_address;
	    uint8_t app_address;
	    uint8_t data_length;
	    uint8_t *data;
	} IR_DATA;
```

The recipient_address is intended to hold the low 16 bits of the badge ID
(obtained via a badge_system_data()-&gt;badgeId) to restrict receipt of the
transmission to the intended badge. More likely you wish to broadcast to any
badges nearby enough to receive, and in that case the value
BADGE_IR_BROADCAST_ID should be used for the recipient address.

The app_address is particular to the app, and is one of the IR_APP values
defined in ir.h:

```
	typedef enum {

	    IR_LED,
	    IR_LIVEAUDIO,
	    IR_PING,

	    IR_APP0,
	    IR_APP1,
	    IR_APP2,
	    IR_APP3,
	    IR_APP4,
	    IR_APP5,
	    IR_APP6,
	    IR_APP7,

	    IR_MAX_ID

	} IR_APP_ID;
```

The data_length is the length of the payload to be transmitted, and the maximum value is 64.
The "data" field is a pointer to the payload buffer.

The function ir_send_complete_message() is used to transmit IR packets.

Here is an example from the "Clue" app showing how IR packets are transmitted:

```
	#include "ir.h"

	[...]

	static void build_and_send_packet(unsigned char address, unsigned short badge_id, uint64_t payload)
	{
	    IR_DATA ir_packet = {
		    .data_length = 8,
		    .recipient_address = badge_id,
		    .app_address = address,
		    .data = (uint8_t *) &payload,
	    };
	    ir_send_complete_message(&ir_packet);
	}

	[... later on ... ]

	uint64_t payload;
	payload = CLUSOE | (uint64_t) playing_as_character;
	build_and_send_packet(BADGE_IR_CLUE_GAME_ADDRESS, BADGE_IR_BROADCAST_ID, payload);
```

## Receiving IR Packets

Receiving IR packets is a little trickier than transmitting them, because incoming
packets are processed by an interrupt.

Some excerpts from the "Clue" badge app illustrate how to do it. At the file
scope we have some variables which maintain a queue of incoming requests.

```
	/* These need to be protected from interrupts. */
	#define QUEUE_SIZE 5
	#define QUEUE_DATA_SIZE 10
	static int queue_in;
	static int queue_out;
	static IR_DATA packet_queue[QUEUE_SIZE] = { {0} };
	static uint8_t packet_data[QUEUE_SIZE][QUEUE_DATA_SIZE] = {{0}};
```

In the initialization part of the app, we register a callback for IR packets:

```
        ir_add_callback(clue_ir_packet_callback, BADGE_IR_CLUE_GAME_ADDRESS);
```

Now "clue_ir_packet_callback" will be called whenever an IR packet arrives.
This function looks like this:

```
	static void clue_ir_packet_callback(const IR_DATA *data)
	{
		// This is called in an interrupt context!
		int next_queue_in;

		next_queue_in = (queue_in + 1) % QUEUE_SIZE;
		if (next_queue_in == queue_out) /* queue is full, drop packet */
			return;
		size_t data_size = data->data_length;
		if (QUEUE_DATA_SIZE < data_size) {
			data_size = QUEUE_DATA_SIZE;
		}
		memcpy(&packet_data[queue_in], data->data, data_size);
		memcpy(&packet_queue[queue_in], data, sizeof(packet_queue[0]));
		packet_queue[queue_in].data = packet_data[queue_in];

		queue_in = next_queue_in;
	}
```

The interrupt handler calls this callback function, passing the IR packet
in as a parameter. This function then puts the packet into our queue if there
is room (dropping the packet if there isn't) and advances the queue in counter.

In our apps main callback function (the one menu.c calls), we monitor the
queue of IR packets and process them:

```
void clue_cb(void)
{
        if (scan_for_incoming_packets)
                clue_check_for_incoming_packets();
        switch (clue_state) {
        case CLUE_INIT:
                clue_init();
	[ ... rest omitted for brevity ... ]
```

The function to process the incoming packets from the queue looks like this:

```
	static void clue_check_for_incoming_packets(void)
	{
	    IR_DATA *new_packet;
	    IR_DATA new_packet_copy;
	    uint8_t packet_data[64];
	    int next_queue_out;
	    uint32_t interrupt_state = hal_disable_interrupts();
	    while (queue_out != queue_in) {
		next_queue_out = (queue_out + 1) % QUEUE_SIZE;
		new_packet = &packet_queue[queue_out];
		queue_out = next_queue_out;
		assert(new_packet->data_length <= 64);
		memcpy(&new_packet_copy, new_packet, sizeof(new_packet_copy));
		new_packet_copy.data = packet_data;
		memcpy(packet_data, new_packet->data, new_packet->data_length);
		hal_restore_interrupts(interrupt_state);
		clue_process_packet(&new_packet_copy);
		interrupt_state = hal_disable_interrupts();
	    }
	    hal_restore_interrupts(interrupt_state);
	}
```

Note that it must disable interrupts while touching the queue to avoid
a race condition with the interrupt handler.  clue_process_packet()
does whatever it needs to with the data.

Badge ID and User Name:
-----------------------

Each badge has a unique ID derived from the processor serial number 
and a user assignable name (using the username badge app).

```
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


```

Audio
-----

TODO: In 2024, there is some PWM stuff that needs to be explained/documented.

See: [source/hal/audio.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/audio.h)

There are two main audio functions:

```
	#include "audio.h"
	void audio_out_beep(uint16_t frequency, uint16_t duration_ms);
	void audio_out_beep_with_cb(uint16_t frequency, uint16_t duration_ms, void (*callback)(void));
```

The first one, audio_out_beep() plays a note of the specified frequency for the specified
duration in milliseconds.  It returns immediately, before the sound has completed playing.
The second function, audio_out_beep_with_callback() does the same, but when the sound finishes
the specified callback function is called.  In this way, you can get the badge to play a
sequence of notes "in the background" like so:

```
	#include "audio.h"

static struct note {
	uint16_t frequency;
	uint16_t duration;
} scale[] = {
	{ 440, 100 }, /* A */
	{ 494, 100 }, /* B */
	{ 523, 100 }, /* C */
	{ 587, 100 }, /* D */
	{ 659, 100 }, /* E */
	{ 698, 100 }, /* F */
	{ 784, 100 }, /* G */
	{ 880, 100 }, /* A */
};

static int current_note = 0;
static void next_note(void)
{
	if (current_note >= 7) {
		current_note = 0;
		return;
	}
	current_note++;
	audio_out_beep_with_cb(scale[current_note].frequency, scale[current_note].duration, next_note);
}

static void play_scale(void)
{
	audio_out_beep_with_cb(scale[current_note].frequency, scale[current_note].duration, next_note);
}
```

Music
-----

The above pattern for playing music is already coded for you
in source/core/music.c and [source/core/music.h](https://github.com/HackRVA/badge2024/blob/main/source/core/music.h)

You could play the scale with this code:

```
	#include "music.h"

	static struct scale_notes = {
		{ NOTE_A3, 100 },
		{ NOTE_B3, 100 },
		{ NOTE_C4, 100 },
		{ NOTE_D4, 100 },
		{ NOTE_E4, 100 },
		{ NOTE_F4, 100 },
		{ NOTE_G4, 100 },
		{ NOTE_A4, 100 },
	};

	static struct tune scale = {
		.num_notes = ARRAYSIZE(scale_notes),
		.note = &scale_notes[0],
	};

	...

	play_tune(&scale, NULL, NULL);
```

The last two arguments of play tune are a callback function and
a cookie.  When the tune reaches the end, the callback function is
called, passing the cookie.  You can use this to arrange measures
of music.  For instance, suppose you have four measures of music
for a 12 bar blues.  You could play in a loop like it like this:

```
extern struct tune one, four, five, turnaround;

void play_blues(void *cookie)
{
	intptr_t measure = (intptr_t) cookie;

	switch (cookie % 12) {
	case 0:
	case 1:
	case 2:
	case 3:
		play_tune(one, play_blues, (void *) (measure + 1));
		break;
	case 4:
	case 5:
		play_tune(four, play_blues, (void *) (measure + 1));
		break;
	case 6:
	case 7:
		play_tune(one, play_blues, (void *) (measure + 1));
		break;
	case 8:
	case 9:
		play_tune(five, play_blues, (void *) (measure + 1));
		break;
	case 10:
		play_tune(one, play_blues, (void *) (measure + 1));
		break;
	case 11:
		play_tune(turnaround, play_blues, (void *) (measure + 1));
		break;
	}
}

```

The first time, you'd call it like:

```
	play_blues(one, play_blues, (void *) 1);
```

Look into gulag.c for an example of how to create a kind of "explosiony" sound.


Setting the Flair LED Color
---------------------------

We have an RGB LED that apps can control.

```
	typedef enum {
	    BADGE_LED_RGB_RED = 0,
	    BADGE_LED_RGB_GREEN,
	    BADGE_LED_RGB_BLUE,
	    BADGE_LED_DISPLAY_BACKLIGHT,
	    BADGE_LED_MAX,
	} BADGE_LED;
```

```
	#include "led_pwm.h"

	void led_pwm_enable(BADGE_LED led, uint8_t duty);
		Sets the brightness of the LED or the display backlight.

	void led_pwm_disable(BADGE_LED led);
		Turns off the LED or the display backlight.
```

Sensors
-------

This year's badge has a few different analog sensors.  The APIs for these sensors
are defined in [source/hal/analog.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/analog.h)
and [source/hal/color_sensor.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/color_sensor.h)

The gist of the analog sensors is you call analog_get_chan_mv() on the appropriate channel
to get a raw value in millivolts, and then use that value with another function to convert
to the appropriate units.

* Thermistor

```
	uint32_t mV = analog_get_chan_mv(ANALOG_CHAN_THERMISTOR);
	int8_t tempC = analog_calc_thermistor_temp_C(mV);
```

* Hall Effect Sensor

```
	uint32_t mV = analog_get_chan_mv(ANALOG_CHAN_HALL_EFFECT);
	int32_t mT = analog_calc_hall_effect_mT(mv);
```

* Conductivity

```
	uint32_t mV = analog_get_chan_mv(ANALOG_CHAN_CONDUCTIVITY);
	float ohms = analog_calc_resistance_ohms(mV);
```

* I2C color sensor

The color sensor returns 5 values for the sensed ambient light, one each for
red, green, blue, white and infrared light.  The values are returned in the
following struct:

```
    struct color_sample {
        uint8_t error_flags;
        uint16_t rgbwi[5];
    };
```

To obtain a sample, call:

```
	struct color_sample s;

	color_sensor_get_sample(&s);
```

* PDM microphone

TODO: document microphone

Flash Memory Access
-------------------

For lower level access to flash API:
see: [source/hal/flash_storage.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/flash_storage.h)

For a higher level flash-backed key-value store API:
see: [source/core/key_value_storage.h](https://github.com/HackRVA/badge2024/blob/main/source/core/key_value_storage.h)

Exiting the Badge App:
----------------------

```
	#include "menu.h"

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

	#include <string.h>
	char *strcpy(char *dest, const char *src);
		Copies NULL terminated string src to dest. There must be enough room in dest.

	char *strcat(char *dest, const char *src);
		Concatenates NULL terminated string src onto the end of NULL terminated string dest.
		There must be enough room in dest.

	#include <stdlib.h>
	int abs(int x);
		Returns the absolute value of integer x.

	#include "trig.h"
	short sine(int a);
	short cosine(int a);
		Returns 256 * the sine and 256 * the cosine of angle a, where a is in units
		of 128th's of a circle. Typical usage is as follows:

		int angle_in_degress = 90; /* degrees */
		int speed = 20;
		int vx = (speed * cosine((128 * angle_in_degrees) / 360)) / 256;
		int vy = (speed * -sine((128 * angle_in_degrees) / 360)) / 256;
	int arctan2(int y, int x);
		The angles returned by arctan2 are in the range -64 to +64, corresponding
		to -180 and +180 degrees.

	#include "fxp_sqrt.h"
	int fxp_sqrt(int x);
		Fixed point square root.
		See: [source/core/fxp_sqrt.h](https://github.com/HackRVA/badge2024/blob/main/source/core/fxp_sqrt.h)


```

(Pseudo)random number generators (PRNGs)
--------------------------------------

There is a hardware based random number generator defined in
[source/hal/random.h](https://github.com/HackRVA/badge2024/blob/main/source/hal/random.h)

```
	void random_insecure_bytes(uint8_t *bytes, size_t len);
```
For most purposes you should probably only use random_insecure_bytes() to
help generate a seed for a pseudorandom number generator.  (I have seen
repeated use of random_insecure_bytes() produce visible patterns. Perhaps
it falls back to a poor pseudorandom number generator if it runs out of
entropy?  Maybe that's why "insecure" is in the name.)

And there is a pseudo-random number generator defined in random.h as well:

```
	#include "random.h"
	uint32_t random_insecure_u32_congruence(uint32_t last);
```

There is another pseudorandom number generator defined in
[source/core/xorshift.h](https://github.com/HackRVA/badge2024/blob/main/source/core/xorshift.h)


```
	#include "xorshift.h"
	unsigned int xorshift(unsigned int *state);
		Returns a pseudo random number derived from seed, *state, and scrambles
		*state to prepare for the next call.  The state variable should be initialized
		to a random seed which should not be zero, and should contain a fair number of
		1 bits and a fair number of 0 bits.

		xorshift has the property that it returns every number representable by 32-bits
		without any repeating in (pseudo)random order, as if you had shuffled a deck of
		approximately 4-billion numbers and then dealt them out one by one, which strictly
		speaking is a fault if you were wanting a truly random sequence of integers, as truly
		random sequences would contain duplicate numbers from time to time.

		For most purposes in games and such, this fault can be ignored.
```

Typical usage:

```

	unsigned int my_state = 0xa5a5a5a5 ^ timestamp; // For example.

	/* Return an integer between 0 and n - 1, inclusive */
	int random_num(int n)
	{
		return (xorshift(&my_state) % n);
	}

```

For more information about how this pseudorandom number generator is implemented,
see https://en.wikipedia.org/wiki/Xorshift#Example_implementation.

Menus
-----

There is a library for badge app menus. The interface for this library is defined in
[source/core/dynmenu.h](https://github.com/HackRVA/badge2024/blob/master/source/core/dynmenu.h)
and the implementation is in [source/core/dynmenu.c](https://github.com/HackRVA/badge2024/blob/master/source/core/dynmenu.c).
The types and functions declared in dynmenu.h and the way they are meant to be used are are
fairly well documented within [the header file](https://github.com/HackRVA/badge2024/blob/master/include/dynmenu.h).

There is also *another*, different menu system defined in
[source/core/menu.h](https://github.com/HackRVA/badge2024/blob/master/source/core/menu.h),
which is used for the badge main menu, and it can also be used by badge apps.
The API for this is a bit more complicated, while at the same time more
limiting, at least in some ways, as it does not (I think) allow for dynamically
changing the menu elements. That is, if you need to have a menu item like "Take X",
where X is replaced at runtime with some other value not known at compile time,
then core/menu.h won't help you (source/apps/maze.c has menu items like this).

The source/core/menu system now has two display methods, one of which uses icons
in a kind of icon carousel.  The icons are drawn with FbDrawObject() (see
above).  See [source/core/menu\_icons.h](https://github.com/HackRVA/badge2024/blob/master/source/core/menu_icon.h).  Menu items are not required to have an icon (though it is best if they
do). If the menu icon for a menu item is NULL, then a blank square will be used as
the default icon.

Pathfinding
-----------

Many games may need to do pathfinding, so an implementation of
the [A-Star algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm) is provided
via [source/core/a_star.h](https://github.com/HackRVA/badge2024/blob/main/source/core/a_star.h)

A simple example demonstrating the use of this a-star implementation is provided in
[source/core/test_a_star.c](https://github.com/HackRVA/badge2024/blob/main/source/core/test_a_star.c)

The game in source/apps/gulag.c uses pathfinding.

Ray Casting
-----------

Games frequently have a need to perform some ray-casting operations using Bresenham's line
drawing algorithm.  For this the bline() function is provided via
[source/core/bline.h](https://github.com/HackRVA/badge2024/blob/main/source/core/bline.h)

```
	#include "bline.h"
	typedef int (*plotting_function)(int x, int y, void *context);

	extern void bline(int x1, int y1, int x2, int y2, plotting_function plot_func, void *context);
```

The user-provided plot_function will be called with the context pointer for every point that
Bresenham's algorithm visits.  An example using bline() can be found in
[source/apps/gulag.c](https://github.com/HackRVA/badge2024/blob/main/source/apps/gulag.c#L2499)
where it is used to do collision detection between bullets and walls.

Some Common Video Game Programming Patterns
-------------------------------------------

## Fixed Point Math

Our processor does not have floating point hardware. However, you may still want to keep track
of things, such as the (x, y) coordinates of an onscreen spaceship, to a greater degree of
precision than an integer would normally allow.  If you use simple integers to track (x, y)
coordinates and velocity vx, vy in a naive way that corresponds directly to screen pixels,
then the slowest speed an object can move is 1 pixel per frame.  The next slowest speed which
may be represented is 2 pixels per frame, which is twice as fast. This situation is not very
precise or satisfactory.  The easiest way to deal with this is with fixed point math. You can
imagine that you could multiply all numbers by 100, and then "just know" that there are two
decimal digits that are implicitly to the right of the decimal point.  This is the basic
idea behind "fixed point math", with one major difference.  To humans using decimal, 100 is a nice
round number.  To the computer using binary, 100 is not a round number.  But 256 is a round
number to the computer.  So we multiply by 256 to shift the "binary point" so that we have 8 bits
implicitly to the right of the "binary point".  So to represent the value 1, you might do:

```
	int x = (1 * 256); /* fixed point value for 1. */
```

This amounts to having 8 bits of data that is to the right of the "decimal point" (or, in this
case the "binary point").

Note that we multiply by 256 rather than explicitly shifting left 8 bits with the "<<" operator.
This is because technically, shifting negative signed integers left is [undefined behavior](https://blog.regehr.org/archives/738).
In any case, the compiler is smart enough to notice that 256 is a power of 2 and will convert this multiplication
to an arithmetic shift left instruction anyway. Likewise a shift right by 8 bits is better
accomplished by dividing by 256, which the compiler will convert to an arithmetic shift right.
(If you forget and use "<<" or ">>" anyway, the badge simulator is compiled with -fsanitize=undefined,
which should catch your mistake and produce a warning at run time.)

Some care must be taken with arithmetic operations.  When adding two fixed point numbers, you can
just add them, but when multiplying two numbers, the result must be shifted right 8 bits (divided by 256),
and when dividing, the result must be shifted left 8 bits (multiplied by 256), and shifts should be done
at appropriate times to keep the calculations in the "sweet spot", avoiding over- or underflows.

Supposing that you have x, y coordinates of a spaceship in fixed point with 8 bits to the right
of the "binary point", when it comes time to use these coordinates to draw the spaceship, you just
shift the values right 8 bits before using them, like so:

```
	FbMove(x / 256, y / 256);
	draw_spaceship();
```

Already discussed above are fixed point sine, cosine and square root functions.  And in these
examples, I've used 8 bits to the right of the "binary point", but it could be any number of bits
you decide it should be.

## Arrays of Things

When making a video game, it is often the case that you want to display and move a relatively
large number of objects around on the screen. The main function for advancing the game through
one frame in your badge app callback function will probably look something like this:

```
	check_for_user_input(); // check which buttons are pressed and change simulation state accordingly
	move_all_the_objects(); // Move all the objects for one iteration of the main game loop
	draw_all_the_objects(); // Draw all the objects in their current locations.
```

The simplest way to accomplish this is with an array.  You might have something like this:

```
	#define MAXOBJECTS 50
	static struct my_game_object {
		int x, y, vx, vy; /* .8 fixed point location and velocity */
		int object_type;
		void (*draw)(struct my_game_object *o);
		void (*move)(struct my_game_object *o);
	} my_object[MAX_OBJECTS];
	static int num_objects = 0;
```

Here, my_object[] is an array of struct my_game_object, num_objects tracks how many objects
are currently in the game (initialized to zero), and MAXOBJECTS is the maximum number of
objects which may exist in the game.

You may then implement move_all_the_objects() and draw_all_the_objects() like so:

```
static void move_all_the_objects(void)
{
	for (int i = 0; i < num_objects; i++)
		my_object[i]->move(&my_object[i]);
}

static void draw_all_the_objects(void)
{
	for (int i = 0; i < num_objects; i++)
		my_object[i]->draw(&my_object[i]);
}
```

Of course the "move" and "draw" function pointers of these objects would have to
be initialized.  Alternatively to using function pointers, you could have a "type" field
in the objects and use a switch statement to call the correct move and draw functions.

## Adding New Objects Into the Array

To add new objects into the array, just add the new one onto the end and
increment num_objects;

```
	int add_object(struct my_game_object o)
	{
		if (num_objects >= MAXOBJECTS)
			return -1;
		my_object[num_objects] = o;
		num_objects++;
	}
```

## Removing Objects from the Array

To remove objects from the array, exchange the object to be deleted with
the last object of the array, and decrement num_objects.

```
	void remove_object(int n)
	{
		if (n < 0 || n >= num_objects)
			return;
		my_object[n] = my_object[num_objects - 1];
		num_objects--;
	}
```

You might use a system like this for bullets for example.  Whenever an enemy,
or the player fires a bullet, you add a new bullet into the array with the
appropriate position and velocity.  Whenever a bullet leaves the screen, or
hits something, and needs to be removed, you remove it from the array.

This all assumes that the order of objects within the array does not matter.
If it does matter in your use case, then you will need to use memmove(), or a
"for" loop to manually shift elements of the array around when deleting
elements to maintain the order.

## Arrays of Different Types of Things

Sometimes having several different arrays, one array for each type
of thing is fine.  For example, in a space invaders game, you might have
one array for all the space invaders, and a second array for all the
bullets.

Other times, you may wish you could have an array which could contain
many different things.  Perhaps you have many different kinds of space
invaders with somewhat different behaviors and data, but you would still
like to think of all of them as "invaders", and be able to call generic
"move" and "draw" functions.

One way to do this is with a "discriminated union".  Suppose
we have three different types of space invaders:

```
	struct invader_crab_data {
		int a, b, c; /* some data specific to crab invaders */
	};

	struct invader_octopus_data {
		int x, y, n, z; /* some data specific to octopus invaders */
	};

	struct invader_stingray_data {
		char data[25]; /* some data specific to stingray invaders */
	};

	/* Make a union to hold any of our 3 types of data */
	union type_specific_data {
		struct invader_crab_data crab;
		struct invader_octopus_data octopus;
		struct invader_stingray_data stingray;
	};

	static struct invader {
		int x, y, vx, vy; /* Data common to all invader types */
		int invader_type; /* 0, 1, or 2 */
		union type_specific_data tsd;
	} invader[MAXINVADERS];
	static int num_invaders = 0;
```

Then, we can do different things depending on the invader_type:

```
	static void do_invader_stuff(struct invader *o)
	{
		/* stuff common to all invaders */
		o->x += o->vx;
		o->y += o->vy;

		/* type specific stuff */
		switch (o->invader_type) {
		case 0:
			do_crab_stuff(o);
			break;
		case 1:
			do_octopus_stuff(o);
			break;
		case 2:
			do_stingray_stuff(o);
			break;
		}
	}
```
		
## Collision detection

The simplest form of collision detection is just to check the distance between
objects.  If the objects are 2D and roughly rectangular in shape, you can do
bounding box checks.  Assuming that objects "a", and "b" are 10x10 squares
centered on their .8 fixed point coordinates:

```
	static int collides(struct my_object *a, struct my_object *b)
	{
		int x1, y1, x2, y2, x3, y3, x4, y4;

		x1 = (a->x >> 8) - 5;
		y1 = (a->y >> 8) - 5;
		x2 = (a->x >> 8) + 5;
		y2 = (a->y >> 8) + 5;

		x3 = (b->x >> 8) - 5;
		y3 = (b->y >> 8) - 5;
		x4 = (b->x >> 8) + 5;
		y4 = (b->y >> 8) + 5;

		if (x1 < x3 && x2 < x3) /* a is to the left of b */
			return 0;
		if (x1 > x4 && x2 > x4) /* a is to the right of b */
			return 0;
		if (y1 < y3 && y2 < y3) /* a is above b */
			return 0;
		if (y1 > y4 && y2 > y4) /* a is below b */
			return 0;
		return 1;
	}
```

Or, maybe you want to calculate the distance between objects, and if the distance
is below some threshold, count it as a collision.  As an optimization, you can
avoid the square root and compare the squared distance to the squared threshold.

```
	static int proximity_collides(struct my_object *a, struct my_object *b, int min_dist)
	{
		int dx, dy, dx2, dy2, threshold2;

		dx = a->x - b->x;
		dy = a->y - b->y;

		dx2 = (dx * dx) >> 8;
		dy2 = (dy * dy) >> 8;

		threshold2 = (min_dist * min_dist) >> 8;

		return (dx2 + dy2) < threshold2;
	}
```

Note: If you need to test each object for collisions with every other object, this can
be problematic depending on how many objects you have.  To explicitly test each object
against every other object is an O(N<sup>2</sup>) algorithm.  If you only have a few objects,
say 10, this would mean 100 collision tests, which is probably ok.  If you have say, 100 objects,
then this would lead to 10000 collision tests, which is probably not ok. If you need to
collision test a large number of objects against a large number of objects, then you will
need some sort of space partitioning scheme to reduce the total number of tests by essentially
caching information about which objects are "near" eachother and only collision testing
"near" objects with each other. Exactly how to do this will depend on the application and is
beyond the scope of this document. Google "space partitioning collision detection".

## A few miscellaneous debugging tips

* Address Sanitizer and Undefined Behavior Sanitizer

  The badge simulator is compiled with debugging information and with the address sanitizer and
  the undefined behavior sanitizer enabled, which should help to catch bugs early and enable
  easy debugging with gdb.

  By default, if the sanitizers catch something, they print messages and halt the program.
  If you do the following:

```
	export ASAN_OPTIONS=abort_on_error=1:halt_on_error=1
	export UBSAN_OPTIONS=abort_on_error=1:halt_on_error=1
```

  it will make the sanitizers trap rather than halt, which is much, much better when
  running under gdb, as you can continue debugging, print back traces, values, etc,
  which you cannot if the program has halted.

* clang scan-build static analyzer

  For the simulator code, you can use clang's static analyzer to help find bugs.
  There's a script in the top level directory,
  [scan_build.sh](https://github.com/HackRVA/badge2024/blob/main/scan_build.sh) to invoke it.

* gdb "TUI mode"

  In gdb, so-called "tui mode" can be activated by pressing "Ctrl-x" then "a" (I remember this
  by the mnemonic "eXtra Awesome mode").  You may occasionally need to refresh the screen if
  it is disturbed by output of your program by pressing Ctrl-L.

  You can switch between source, assembly and combined views by the command "layout next".

* Serial debug via USB

  When running the hardware badge, printf statements will be sent via USB serial back to your
  computer.  If you find a problem which manifests on the physical badge, but not on the
  simulator, judicious use of printfs may help you find the problem.  You can see this output
  by using a terminal program.  On linux, minicom works reasonably well:

```
	minicom -D /dev/ttyACM0 -8
```

  You may need to install minicom.

