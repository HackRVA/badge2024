/* MIT License
 * Copyright (c) 2021, Michal Kozakiewicz, github.com/michal037
 *
 * code repository: https://github.com/michal037/driver-ST7735S
 * code version: 3.0.0
 *
 * There are many references to ST7735S Datasheet in this code,
 * please provide it as well.
 * When updating a PDF, update all references to the appropriate
 * version and page. This will avoid delivering multiple PDF files.
 * Convention used: (pdf VERSION PAGE/S)
 * Convention example: (pdf v1.4 p10) or (pdf v1.4 p68-70)
 */

/* about size_t: https://stackoverflow.com/a/131833 */

/* from stdlib.h: size_t, malloc, free */
#include <stdlib.h>

#ifndef _LIBRARY_ST7735S_
#define _LIBRARY_ST7735S_

/* about extern "C": https://stackoverflow.com/q/3789340 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LCD_SLEEP_IN  1 /* (true)  turn on sleep mode */
#define LCD_SLEEP_OUT 0 /* (false) turn off sleep mode */

#define LCD_DISPLAY_ON  1 /* (true)  turn on the display */
#define LCD_DISPLAY_OFF 0 /* (false) turn off the display */

#define LCD_IDLE_ON  1 /* (true)  turn on idle mode */
#define LCD_IDLE_OFF 0 /* (false) turn off idle mode */

#define LCD_INVERSION_ON  1 /* (true)  turn on inversion */
#define LCD_INVERSION_OFF 0 /* (false) turn off inversion */

/* flags for lcd_setMemoryAccessControl(); see (pdf v1.4 p142)
 *
 * For a description of how to use these flags,
 * see the documentation for the lcd_setMemoryAccessControl() function.
 */
#define LCD_MADCTL_MY  (1<<7) /* Row Address Order */
#define LCD_MADCTL_MX  (1<<6) /* Column Address Order */
#define LCD_MADCTL_MV  (1<<5) /* Row / Column Exchange */
#define LCD_MADCTL_ML  (1<<4) /* Vertical Refresh Order */
#define LCD_MADCTL_BGR (1<<3) /* RGB or BGR Order */
#define LCD_MADCTL_MH  (1<<2) /* Horizontal Refresh Order */
#define LCD_MADCTL_DEFAULT 0  /* Default state */

/* arguments for lcd_setTearingEffectLine(); see (pdf v1.4 p140) */
#define LCD_TEARING_MODE_V    0 /* turn on tearing with V-Blanking */
#define LCD_TEARING_MODE_VH   1 /* turn on tearing with V-Blanking H-Blanking */
#define LCD_TEARING_OFF     255 /* turn off tearing effect line */

/* arguments for lcd_setGammaPredefined(); see (pdf v1.4 p125) */
#define LCD_GAMMA_PREDEFINED_1 (1<<0) /* Gamma Curve 1 */
#define LCD_GAMMA_PREDEFINED_2 (1<<1) /* Gamma Curve 2 */
#define LCD_GAMMA_PREDEFINED_3 (1<<2) /* Gamma Curve 3 */
#define LCD_GAMMA_PREDEFINED_4 (1<<3) /* Gamma Curve 4 */

/* arguments for lcd_setInterfacePixelFormat(); see (pdf v1.4 p150) */
#define LCD_PIXEL_FORMAT_444 3 /* 12-bit/pixel */
#define LCD_PIXEL_FORMAT_565 5 /* 16-bit/pixel */
#define LCD_PIXEL_FORMAT_666 6 /* 18-bit/pixel */

/* ########################################################################## */

/* status code from function
 * - anything less than LCD_OK are errors
 * - LCD_OK means everything is fine
 * - anything above LCD_OK means all fine plus additional information
 */
typedef enum lcd_status_t {
    LCD_FAIL = -1,
    LCD_OK   =  0 /* have to be 0 */
} lcd_status_t;

/* MODIFICATIONS HERE HAVE TO BE ALSO MADE TO lcd_createSettings() FUNCTION! */
typedef struct lcd_t {
    /* I assume 'unsigned short int' is 16 bits wide. */
    /* I do not want to store information about the SPI interface.
     * This information is kept by the user of this library.
     * Before communicating with the driver, the user manually activates
     * the appropriate chip select line. After communication with the driver,
     * the user manually deactivates the appropriate chip select line.
     * This approach allows for easier combination of multiple devices
     * on one SPI bus and relieves the library from managing it.
     *
     * ST7735S uses MODE_0 SPI and MSB (bit) is transmitted first.
     * (pdf v1.4 p36) (pdf v1.4 p44)
     */

    /* ###################################################################### */

    /* I suppose someone will want to use this library with slight tweaks
     * for a different driver. That is why I use two-byte wide
     * variables for display size.
     *
     * width_offset  :: column :: X
     * height_offset :: row    :: Y
     */
    unsigned short int width;
    unsigned short int height;
    unsigned short int width_offset;
    unsigned short int height_offset;

    /* These variables store the pin numbers.
     * two-byte wide variables for devices with a large number of pins
     */
    unsigned short int pin_communicationMode; /* data or command */
    signed   short int pin_reset; /* set to -1 if unused */

    /* The data are sent to the display driver.
     * What is the state of the 'pin_communicationMode'?
     * HIGH state is 1; positive voltage
     * LOW state is 0; ground voltage
     * ST7735S wants the data to be transferred with HIGH state. (pdf v1.4 p24)
     * This variable is automatically set in lcd_createSettings().
     */
    unsigned char dataMode_activeState;

    /* Hardware reset activation.
     * What is the state of the 'pin_reset'?
     * HIGH state is 1; positive voltage
     * LOW state is 0; ground voltage
     * ST7735S activates reset with LOW state. (pdf v1.4 p23)
     * This variable is automatically set in lcd_createSettings().
     */
    unsigned char reset_activeState;

    /* This flag remembers the last state of MADCTL_MV.
     * It is needed to decide whether to swap width with height.
     * Used in lcd_setMemoryAccessControll() function.
     */
    unsigned char flag_madctl_mv;

    /* last set interface pixel format for drawing functions */
    unsigned char interface_pixel_format;
} lcd_t;

/* lcd_ptr_t is pointer to lcd_t struct */
typedef struct lcd_t * lcd_ptr_t;

/* This global variable stores a pointer to struct with the settings of
 * the active display.
 * Use this to quickly change the settings by replacing this pointer or
 * changing variables pointed by this pointer.
 */
extern lcd_ptr_t lcd_settings;

/* ########################################################################## */

/* #### YOU HAVE TO IMPLEMENT THIS FUNCTION ####
 *
 * Description:
 *   Pauses the program for the amount of time (in milliseconds) specified
 *   as parameter. (There are 1000 milliseconds in a second.)
 *
 * Parameters:
 * - milliseconds: the number of milliseconds to pause.
 *
 * Returns:
 *   Nothing
 */
void lcd_delay(unsigned long int milliseconds);

/* #### YOU HAVE TO IMPLEMENT THIS FUNCTION ####
 *
 * Description:
 *   Write a HIGH or a LOW value to a digital pin.
 *   HIGH is treated as a positive voltage.
 *   LOW is treated as a ground voltage (0V).
 *
 * Parameters:
 * - pin: number of pin on your device
 * - value: 1 for HIGH; 0 for LOW
 *
 * Returns:
 *   Nothing
 */
void lcd_digitalWrite(unsigned short int pin, unsigned char value);

/* #### YOU HAVE TO IMPLEMENT THIS FUNCTION ####
 *
 * Description:
 *   Write 'length' bytes from 'buffer' to SPI.
 *   The 'length' must be equal to or less than the actual 'buffer' capacity.
 *   WARNING: THE DATA IN THE BUFFER CAN BE OVERWRITTEN!
 *   WARNING: THE LENGTH CANNOT EXCEED THE BUFFER CAPACITY!
 *
 * Parameters:
 * - buffer: pointer to memory block with data
 * - length: 'length' bytes to write
 *
 * Returns:
 *   Nothing
 */
void lcd_spiWrite(unsigned char* buffer, size_t length);

/* Description:
 *   Create new display settings. This function allocates memory.
 *
 *   WARNING: lcd_setMemoryAccessControl() and lcd_setInterfacePixelFormat()
 *   HAVE TO BE REFRESHED IN INITIALIZATION TO PREVENT BUGS!
 *
 * Parameters:
 * - width: display width in pixels
 * - height: display height in pixels
 * - width_offset: display width offset in pixels
 * - height_offset: display height offset in pixels
 * - pin_communicationMode: The pin number of your device that controls
 *   the sending of data or commands to the display.
 * - pin_reset: The pin number of your device that resets the display.
 *
 * Returns:
 *   pointer to lcd_t structure or NULL if memory allocation failed
 */
lcd_ptr_t lcd_createSettings(
        unsigned short int width,
        unsigned short int height,
        unsigned short int width_offset,
        unsigned short int height_offset,
        unsigned short int pin_communicationMode,
        signed   short int pin_reset
);

/* Description:
 *   Delete display settings. This function frees memory.
 *
 * Parameters:
 * - settings: pointer to lcd_t structure
 *
 * Returns:
 *   Nothing
 */
void lcd_deleteSettings(lcd_ptr_t settings);

/* Description:
 *   Set display settings as active.
 *
 * Parameters:
 * - settings: pointer to lcd_t structure
 *
 * Returns:
 *   Nothing
 */
void lcd_setSettingsActive(lcd_ptr_t settings);

/* Description:
 *   Get active display settings.
 *
 * Parameters:
 *   Nothing
 *
 * Returns:
 *   pointer to lcd_t structure
 */
lcd_ptr_t lcd_getSettingsActive();

/* Description:
 *   Write data to the display driver.
 *
 *   Write 'length' bytes from 'buffer' to SPI.
 *   The 'length' must be equal to or less than the actual 'buffer' capacity.
 *   WARNING: THE DATA IN THE BUFFER CAN BE OVERWRITTEN!
 *   WARNING: THE LENGTH CANNOT EXCEED THE BUFFER CAPACITY!
 *
 * Parameters:
 * - buffer: pointer to memory block with data
 * - length: 'length' bytes to write
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_writeData(unsigned char* buffer, size_t length);

/* Description:
 *   Write command to the display driver.
 *
 *   Write 'length' bytes from 'buffer' to SPI.
 *   The 'length' must be equal to or less than the actual 'buffer' capacity.
 *   WARNING: THE DATA IN THE BUFFER CAN BE OVERWRITTEN!
 *   WARNING: THE LENGTH CANNOT EXCEED THE BUFFER CAPACITY!
 *
 * Parameters:
 * - buffer: pointer to memory block with command
 * - length: 'length' bytes to write
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_writeCommand(unsigned char* buffer, size_t length);

/* Description:
 *   Write command to the display driver.
 *
 * Parameters:
 * - command: driver command
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_writeCommandByte(unsigned char command);

/* Description:
 *   Apply a hardware reset. 121 milliseconds for ST7735S.
 *
 * Parameters:
 *   Nothing
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_hardwareReset();

/* Description:
 *   Apply a software reset. 120 milliseconds for ST7735S.
 *
 * Parameters:
 *   Nothing
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_softwareReset();

/* Description:
 *   Display initialization. It have to be done for each display separately.
 *   Currently disables hardware reset before using the display.
 *
 * Parameters:
 *   Nothing
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_initialize();

/* Description:
 *   Sleep mode management.
 *   After 'Power On' or 'Hardware reset' or 'Software reset'
 *   display driver is in 'Sleep In' mode. (pdf v1.4 p89-91)
 *   Sleep mode reduces power consumption. (pdf v1.4 p30) (pdf v1.4 p87)
 *   120 milliseconds for ST7735S.
 *
 * Parameters:
 * - sleep: Anything that evaluates to true or false. Prepared definitions:
 *   LCD_SLEEP_IN  - (true)  turn on sleep mode
 *   LCD_SLEEP_OUT - (false) turn off sleep mode
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setSleepMode(unsigned char sleep);

/* Description:
 *   Idle mode management.
 *   After 'Power On' or 'Hardware reset' or 'Software reset'
 *   idle mode is off. (pdf v1.4 p89-91)
 *   Idle mode reduces power consumption. (pdf v1.4 p30) (pdf v1.4 p87)
 *
 *   Idle Mode Off (pdf v1.4 p147):
 *   1. LCD can display 4096, 65k or 262k colors.
 *   2. Normal frame frequency is applied.
 *
 *   Idle Mode On (pdf v1.4 p148):
 *   1. Color expression is reduced. The primary and the secondary colors using
 *      MSB of each R, G and B in the Frame Memory, 8 color depth data
 *      are displayed.
 *   2. 8-Color mode frame frequency is applied.
 *
 * Parameters:
 * - idle: Anything that evaluates to true or false. Prepared definitions:
 *   LCD_IDLE_ON  - (true)  turn on idle mode
 *   LCD_IDLE_OFF - (false) turn off idle mode
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setIdleMode(unsigned char idle);

/* Description:
 *   Display mode management.
 *   After 'Power On' or 'Hardware reset' or 'Software reset'
 *   display is off. (pdf v1.4 p89-91)
 *
 * Parameters:
 * - display: Anything that evaluates to true or false. Prepared definitions:
 *   LCD_DISPLAY_ON  - (true)  turn on the display
 *   LCD_DISPLAY_OFF - (false) turn off the display
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setDisplayMode(unsigned char display);

/* Description:
 *   Display inversion management.
 *
 * Parameters:
 * - inversion: Anything that evaluates to true or false. Prepared definitions:
 *   LCD_INVERSION_ON  - (true) turn on inversion
 *   LCD_INVERSION_OFF - (false) turn off inversion
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setDisplayInversion(unsigned char inversion);

/* Description:
 *   Set predefined gamma correction. The gamma correction profile depends
 *   on the setting of the physical GS line.
 *   Arguments cannot be combined together.
 *
 * Parameters:
 * - gamma: Predefined gamma curve. Prepared definitions:
 *   LCD_GAMMA_PREDEFINED_1 - GS_1: 2.2; or GS_0: 1.0;
 *   LCD_GAMMA_PREDEFINED_2 - GS_1: 1.8; or GS_0: 2.5;
 *   LCD_GAMMA_PREDEFINED_3 - GS_1: 2.5; or GS_0: 2.2;
 *   LCD_GAMMA_PREDEFINED_4 - GS_1: 1.0; or GS_0: 1.8;
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setGammaPredefined(unsigned char gamma);

/* Description:
 *   Management of the tearing effect line. It is a physical line to connect to
 *   the MCU to help synchronize the image. (pdf v1.4 p140)
 *   Arguments cannot be combined together.
 *
 * Parameters:
 * - tearing: line signal control. Prepared definitions:
 *   LCD_TEARING_MODE_V  - turn on tearing with V-Blanking
 *   LCD_TEARING_MODE_VH - turn on tearing with V-Blanking and H-Blanking
 *   LCD_TEARING_OFF     - turn off tearing effect line
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setTearingEffectLine(unsigned char tearing);

/* Description:
 *   This function sets the MADCTL register in the display driver. It controls
 *   the direction in which the image is displayed, the way the display
 *   is refreshed and the order of RGB bytes. (pdf v1.4 p142)
 *
 *   The default state of this register for the ST7735S driver is
 *   all flags disabled. This means that when you use the flags below, you are
 *   just turning them on. Based on the Reset Table, see: (pdf v1.4 p89-91)
 *
 *   For a description of the MY, MX, and MV flags that control the display
 *   direction of the image, see (pdf v1.4 p77-78) and (pdf v1.4 p68-70) and
 *   (pdf v1.4 p74-76).
 *   Changing MV flag requires the correction of the values of variables
 *   holding the width and height of the display and their offsets. This is done
 *   automatically by the library.
 *
 *   By default, the RGB sequence is used. By setting the LCD_MADCTL_BGR flag,
 *   you select the BGR order. This flag does not change the way data
 *   are transferred. It is for the physical connection of the driver
 *   with the display which may differ depending on the display module.
 *
 *   You can combine flags together using the addition operator '+'
 *   or bitwise inclusive OR '|'.
 *
 *   WARNING: HAVE TO BE REFRESHED IN INITIALIZATION TO PREVENT BUGS!
 *
 * Parameters:
 * - flags: The flags to be set. Prepared definitions:
 *   LCD_MADCTL_MY  (1<<7) Row Address Order
 *   LCD_MADCTL_MX  (1<<6) Column Address Order
 *   LCD_MADCTL_MV  (1<<5) Row / Column Exchange
 *   LCD_MADCTL_ML  (1<<4) Vertical Refresh Order
 *   LCD_MADCTL_BGR (1<<3) RGB or BGR ORDER
 *   LCD_MADCTL_MH  (1<<2) Horizontal Refresh Order
 *   LCD_MADCTL_DEFAULT 0  Default state
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setMemoryAccessControl(unsigned char flags);

/* Description:
 *   Set the way pixel data are sent to the display driver.
 *   Arguments cannot be combined together.
 *
 *   WARNING: HAVE TO BE REFRESHED IN INITIALIZATION TO PREVENT BUGS!
 *
 * Parameters:
 * - format: Interface Pixel Format. Prepared definitions:
 *   LCD_PIXEL_FORMAT_444 - 12-bit/pixel
 *   LCD_PIXEL_FORMAT_565 - 16-bit/pixel
 *   LCD_PIXEL_FORMAT_666 - 18-bit/pixel
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setInterfacePixelFormat(unsigned char format);

/* Description:
 *   Set the window where you will draw. After setting the window, you can
 *   enable memory write and start sending data to the display driver.
 *
 *   column ranges: (pdf v1.4 p105) (pdf v1.4 p128)
 *   'column_start' always must be less than or equal to 'column_end'.
 *   When 'column_start' or 'column_end' are greater than maximum column
 *   address, data of out of range will be ignored.
 *   0 <= column_start <= column_end <= COLUMN_MAX
 *   width_offset :: column :: X
 *
 *   row ranges: (pdf v1.4 p105) (pdf v1.4 p130)
 *   'row_start' always must be less than or equal to 'row_end'.
 *   When 'row_start' or 'row_end' are greater than maximum row address,
 *   data of out of range will be ignored.
 *   0 <= row_start <= row_end <= ROW_MAX
 *   height_offset :: row :: Y
 *
 * Parameters:
 * - column_start: x0 column line where to start
 * - row_start: y0 row line where to start
 * - column_end: x1 column line where to end; included
 * - row_end: y1 row line where to end; included
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_setWindowPosition(
        unsigned short int column_start,
        unsigned short int row_start,
        unsigned short int column_end,
        unsigned short int row_end
);

/* Description:
 *   Activate memory write. After that, you can start sending data
 *   to the display driver. Before Memory Write activation, you have to set
 *   the Window Position where will you draw.
 *
 * Parameters:
 *   Nothing
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_activateMemoryWrite();

/* ########################################################################## */

/* SCHEME OF USED BITS FOR DRAWING FUNCTIONS [IMPORTANT NOTES]
 *
 * case LCD_PIXEL_FORMAT_444:
 *   (pdf v1.4 p51) 'Note 2: (...) Only complete pixel data will be stored
 *   in the frame memory.'
 *   For 4-line SPI interface the smallest chunk is 1 byte consisting of 8 bits.
 *   The 444 format contains 12 bits per pixel. The smallest chunk of data
 *   in which a pixel can be placed is 2 bytes consisting of 16 bits.
 *   There is a loss of 4 bits when sending 1 pixel. Only an even number of
 *   pixels is efficient for this method and it is certain that it will not
 *   cause errors. I DON'T SUPPORT THE 444 PIXEL FORMAT FOR DRAWING FUNCTIONS.
 *
 *
 * case LCD_PIXEL_FORMAT_565:
 *   4 3 2 1 0 5 4 3    2 1 0 4 3 2 1 0    bits      [R]ed
 *   R R R R R G G G    G G G B B B B B    in        [G]reen
 *   7 6 5 4 3 2 1 0    7 6 5 4 3 2 1 0    buffer    [B]lue
 *   1st byte           2nd byte
 *
 *   MSB 7 6 5 4 3 2 1 0 LSB
 *       X X X X X - - -
 *   'X' bit is used; '-' bit is unused;
 *   scheme of used bits from input variables: Red Blue
 *
 *   MSB 7 6 5 4 3 2 1 0 LSB
 *       X X X X X X - -
 *   'X' bit is used; '-' bit is unused;
 *   scheme of used bits from input variable: Green
 *
 *
 * case LCD_PIXEL_FORMAT_666:
 *   5 4 3 2 1 0 - -    5 4 3 2 1 0 - -    5 4 3 2 1 0 - -    bits      [R]ed
 *   R R R R R R - -    G G G G G G - -    B B B B B B - -    in        [G]reen
 *   7 6 5 4 3 2 1 0    7 6 5 4 3 2 1 0    7 6 5 4 3 2 1 0    buffer    [B]lue
 *   1st byte           2nd byte           3rd byte
 *
 *   MSB 7 6 5 4 3 2 1 0 LSB
 *       X X X X X X - -
 *   'X' bit is used; '-' bit is unused;
 *   scheme of used bits from input variables: Red Green Blue
 */

/* Description:
 *   Draw pixel on the screen.
 *
 *   This function supports the following pixel formats:
 *   - LCD_PIXEL_FORMAT_565
 *   - LCD_PIXEL_FORMAT_666
 *   Check 'SCHEME OF USED BITS FOR DRAWING FUNCTIONS' (above) to know which
 *   bits are used for the color. In brief: the most significant.
 *
 *   This function is slow, but checks for errors at every step.
 *   The faster way to draw is to send part of the image in a custom buffer.
 *   The fastest way to draw is to send whole frames at once.
 *
 * Parameters:
 * - x: Starting point on the horizontal axis.
 * - y: Starting point on the vertical axis.
 * - red: The red color intensity of the pixel.
 * - green: The green color intensity of the pixel.
 * - blue: The blue color intensity of the pixel.
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_drawPixel(
        unsigned short int x,
        unsigned short int y,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
);

/* Description:
 *   Draw horizontal line on the screen.
 *
 *   This function supports the following pixel formats:
 *   - LCD_PIXEL_FORMAT_565
 *   - LCD_PIXEL_FORMAT_666
 *   Check 'SCHEME OF USED BITS FOR DRAWING FUNCTIONS' (above) to know which
 *   bits are used for the color. In brief: the most significant.
 *
 *   This function is slow, but checks for errors at every step.
 *   The faster way to draw is to send part of the image in a custom buffer.
 *   The fastest way to draw is to send whole frames at once.
 *
 * Parameters:
 * - x0: Starting point on the horizontal axis.
 * - y0: Starting point on the vertical axis.
 * - x1: Ending point on the horizontal axis, included.
 * - red: The red color intensity of the pixel.
 * - green: The green color intensity of the pixel.
 * - blue: The blue color intensity of the pixel.
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_drawHorizontalLine(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int x1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
);

/* Description:
 *   Draw vertical line on the screen.
 *
 *   This function supports the following pixel formats:
 *   - LCD_PIXEL_FORMAT_565
 *   - LCD_PIXEL_FORMAT_666
 *   Check 'SCHEME OF USED BITS FOR DRAWING FUNCTIONS' (above) to know which
 *   bits are used for the color. In brief: the most significant.
 *
 *   This function is slow, but checks for errors at every step.
 *   The faster way to draw is to send part of the image in a custom buffer.
 *   The fastest way to draw is to send whole frames at once.
 *
 * Parameters:
 * - x0: Starting point on the horizontal axis.
 * - y0: Starting point on the vertical axis.
 * - y1: Ending point on the vertical axis, included.
 * - red: The red color intensity of the pixel.
 * - green: The green color intensity of the pixel.
 * - blue: The blue color intensity of the pixel.
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_drawVerticalLine(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int y1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
);

/* Description:
 *   Draw rectangle on the screen.
 *
 *   This function supports the following pixel formats:
 *   - LCD_PIXEL_FORMAT_565
 *   - LCD_PIXEL_FORMAT_666
 *   Check 'SCHEME OF USED BITS FOR DRAWING FUNCTIONS' (above) to know which
 *   bits are used for the color. In brief: the most significant.
 *
 *   This function is slow, but checks for errors at every step.
 *   The faster way to draw is to send part of the image in a custom buffer.
 *   The fastest way to draw is to send whole frames at once.
 *
 * Parameters:
 * - x0: Starting point on the horizontal axis.
 * - y0: Starting point on the vertical axis.
 * - x1: Ending point on the horizontal axis, included.
 * - y1: Ending point on the vertical axis, included.
 * - red: The red color intensity of the pixel.
 * - green: The green color intensity of the pixel.
 * - blue: The blue color intensity of the pixel.
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_drawRectangle(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int x1,
        unsigned short int y1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
);

/* Description:
 *   Draw filled rectangle on the screen.
 *
 *   This function supports the following pixel formats:
 *   - LCD_PIXEL_FORMAT_565
 *   - LCD_PIXEL_FORMAT_666
 *   Check 'SCHEME OF USED BITS FOR DRAWING FUNCTIONS' (above) to know which
 *   bits are used for the color. In brief: the most significant.
 *
 *   This function is slow, but checks for errors at every step.
 *   The faster way to draw is to send part of the image in a custom buffer.
 *   The fastest way to draw is to send whole frames at once.
 *
 * Parameters:
 * - x0: Starting point on the horizontal axis.
 * - y0: Starting point on the vertical axis.
 * - x1: Ending point on the horizontal axis, included.
 * - y1: Ending point on the vertical axis, included.
 * - red: The red color intensity of the pixel.
 * - green: The green color intensity of the pixel.
 * - blue: The blue color intensity of the pixel.
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_drawFilledRectangle(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int x1,
        unsigned short int y1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
);

/* Description:
 *   Clear the screen.
 *
 *   This function supports the following pixel formats:
 *   - LCD_PIXEL_FORMAT_565
 *   - LCD_PIXEL_FORMAT_666
 *   Check 'SCHEME OF USED BITS FOR DRAWING FUNCTIONS' (above) to know which
 *   bits are used for the color. In brief: the most significant.
 *
 *   This function is slow, but checks for errors at every step.
 *   The faster way to draw is to send part of the image in a custom buffer.
 *   The fastest way to draw is to send whole frames at once.
 *
 * Parameters:
 * - red: The red color intensity of the pixel.
 * - green: The green color intensity of the pixel.
 * - blue: The blue color intensity of the pixel.
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_clearScreen(
        unsigned char red,
        unsigned char green,
        unsigned char blue
);

/* Description:
 *   Send frame buffer to the display driver.
 *   Some libraries refuses to send a large buffer at one time. This function
 *   breaks up a large block of memory into chunks that library is able
 *   to transfer the fastest.
 *
 *   WARNING: THE DATA IN THE BUFFER CAN BE OVERWRITTEN!
 *   WARNING: THE LENGTH_BUFFER CANNOT EXCEED THE BUFFER CAPACITY!
 *
 * Parameters:
 * - buffer: pointer to memory block with frame data
 * - length_buffer: total number of bytes to send
 * - length_chunk: number of bytes to send as chunk
 *
 * Returns:
 *   status code indicating success or failure
 */
lcd_status_t lcd_framebuffer_send(
        unsigned char * buffer,
        const    size_t length_buffer,
        const    size_t length_chunk
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LIBRARY_ST7735S_ */