
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
 *
 * Adapted for badge display interface for HackRVA's badge.
 */

#include <string.h> /* memcpy */
#include "display_st7735_rp2040_internal.h"

#define HIGH 1
#define LOW  0

/* documented in header; see struct lcd_t */
#define DATAMODE_ACTIVESTATE HIGH
#define RESET_ACTIVESTATE    LOW

/* MV flag default state based on the Reset Table, see: (pdf v1.4 p89-91) */
#define FLAG_MADCTL_MV_DEFAULT 0

/* default pixel format; Based on the Reset Table, see: (pdf v1.4 p89-91) */
#define INTERFACE_PIXEL_FORMAT_DEFAULT LCD_PIXEL_FORMAT_666

/* ST7735S driver commands (pdf v1.4 p5) */
#define LCD_CMD_SWRESET 0x01 /* Software Reset (pdf v1.4 p108) */
#define LCD_CMD_SLPIN   0x10 /* Sleep In (pdf v1.4 p119) */
#define LCD_CMD_SLPOUT  0x11 /* Sleep Out (pdf v1.4 p120) */
#define LCD_CMD_INVOFF  0x20 /* Display Inversion Off (pdf v1.4 p123) */
#define LCD_CMD_INVON   0x21 /* Display Inversion On (pdf v1.4 p124) */
#define LCD_CMD_GAMSET  0x26 /* Gamma Set (pdf v1.4 p125) */
#define LCD_CMD_DISPOFF 0x28 /* Display Off (pdf v1.4 p126) */
#define LCD_CMD_DISPON  0x29 /* Display On (pdf v1.4 p127) */
#define LCD_CMD_CASET   0x2A /* Column Address Set (pdf v1.4 p128-129) */
#define LCD_CMD_RASET   0x2B /* Row Address Set (pdf v1.4 p130-131) */
#define LCD_CMD_RAMWR   0x2C /* Memory Write (pdf v1.4 p132) */
#define LCD_CMD_TEOFF   0x34 /* Tearing Effect Line OFF (pdf v1.4 p139) */
#define LCD_CMD_TEON    0x35 /* Tearing Effect Line ON (pdf v1.4 p140) */
#define LCD_CMD_MADCTL  0x36 /* Memory Data Access Control (pdf v1.4 p142) */
#define LCD_CMD_IDMOFF  0x38 /* Idle Mode Off (pdf v1.4 p147) */
#define LCD_CMD_IDMON   0x39 /* Idle Mode On (pdf v1.4 p148) */
#define LCD_CMD_COLMOD  0x3A /* Interface Pixel Format (pdf v1.4 p150) */

/* global variable with settings of the active display module */
lcd_ptr_t lcd_settings = NULL;

lcd_ptr_t lcd_createSettings(
        unsigned short int width,
        unsigned short int height,
        unsigned short int width_offset,
        unsigned short int height_offset,
        unsigned short int pin_communicationMode,
        signed   short int pin_reset
) {
    lcd_ptr_t settings = malloc(sizeof(struct lcd_t));

    /* if out of RAM memory */
    if(settings == NULL) {
        return NULL;
    }

    settings->width  = width;
    settings->height = height;
    settings->width_offset  = width_offset;  /* width_offset  :: column :: X */
    settings->height_offset = height_offset; /* height_offset :: row    :: Y */

    settings->pin_communicationMode = pin_communicationMode;
    settings->pin_reset             = pin_reset;

    settings->dataMode_activeState = DATAMODE_ACTIVESTATE;
    settings->reset_activeState    = RESET_ACTIVESTATE;

    /* HAVE TO BE REFRESHED IN INITIALIZATION TO PREVENT BUGS!
     * Use: lcd_setMemoryAccessControl()
     */
    settings->flag_madctl_mv = FLAG_MADCTL_MV_DEFAULT;

    /* HAVE TO BE REFRESHED IN INITIALIZATION TO PREVENT BUGS!
     * Use: lcd_setInterfacePixelFormat()
     */
    settings->interface_pixel_format = INTERFACE_PIXEL_FORMAT_DEFAULT;

    return settings;
}

void lcd_deleteSettings(lcd_ptr_t settings) {
    /* There is no need to check if the pointer is NULL before calling
     * the free() function
     */

    /* Prevent a bug If someone tries to draw through dangling pointer. */
    if(settings == lcd_settings) {
        lcd_settings = NULL;
    }

    /* previously allocated by lcd_createSettings() */
    free(settings);
}

void lcd_setSettingsActive(lcd_ptr_t settings) {
    /* only rewrite; allow NULL pointer */
    lcd_settings = settings;
}

lcd_ptr_t lcd_getSettingsActive() {
    return lcd_settings;
}

lcd_status_t lcd_writeData(unsigned char* buffer, size_t length) {
    /* check inputs */
    if(buffer == NULL) {
        return LCD_FAIL;
    }
    if(length <= 0) {
        return LCD_FAIL;
    }

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* write data to the display driver */
    lcd_digitalWrite(
            lcd_settings->pin_communicationMode,
            lcd_settings->dataMode_activeState
    );
    lcd_spiWrite(buffer, length);

    return LCD_OK;
}

lcd_status_t lcd_writeCommand(unsigned char* buffer, size_t length) {
    /* check inputs */
    if(buffer == NULL) {
        return LCD_FAIL;
    }
    if(length <= 0) {
        return LCD_FAIL;
    }

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* write command to the display driver */
    lcd_digitalWrite(
            lcd_settings->pin_communicationMode,
            !( lcd_settings->dataMode_activeState ) /* command is negative state */
    );
    lcd_spiWrite(buffer, length);

    return LCD_OK;
}

lcd_status_t lcd_writeCommandByte(unsigned char command) {
    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* write command to the display driver */
    lcd_digitalWrite(
            lcd_settings->pin_communicationMode,
            !( lcd_settings->dataMode_activeState ) /* command is negative state */
    );
    lcd_spiWrite(&command, sizeof(command));

    return LCD_OK;
}

lcd_status_t lcd_hardwareReset() {
    /* Reset Timing (pdf v1.4 p93) */

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* check if the reset pin is connected */
    if(lcd_settings->pin_reset < 0) {
        return LCD_FAIL;
    }

    /* activate reset */
    lcd_digitalWrite(
            lcd_settings->pin_reset,
            lcd_settings->reset_activeState
    );
    lcd_delay(1); /* 1 millisecond; have to be longer than 9us (pdf v1.4 p93) */

    /* cancel reset */
    lcd_digitalWrite(
            lcd_settings->pin_reset,
            !( lcd_settings->reset_activeState )
    );
    lcd_delay(120); /* 120 milliseconds */

    return LCD_OK;
}

lcd_status_t lcd_softwareReset() {
    if(lcd_writeCommandByte(LCD_CMD_SWRESET) < LCD_OK) {
        return LCD_FAIL;
    }

    /* It is required to wait 120 milliseconds after issuing the command. */
    lcd_delay(120); /* (pdf v1.4 p108) */

    return LCD_OK;
}

lcd_status_t lcd_initialize() {
    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* If the 'pin_reset' is connected (>= 0) then disable hardware reset. */
    if(lcd_settings->pin_reset >= 0) {
        lcd_digitalWrite(
                lcd_settings->pin_reset,
                /* inactive is negative state */
                !( lcd_settings->reset_activeState )
        );
    }

    return LCD_OK;
}

lcd_status_t lcd_setSleepMode(unsigned char sleep) {
    /* This command turns ON or OFF sleep mode. */
    if(lcd_writeCommandByte(sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT) < LCD_OK) {
        return LCD_FAIL;
    }

    /* It is required to wait 120 milliseconds after issuing the command. */
    lcd_delay(120); /* (pdf v1.4 p119-120) */

    return LCD_OK;
}

lcd_status_t lcd_setIdleMode(unsigned char idle) {
    /* This command turns ON or OFF idle mode. */
    if(lcd_writeCommandByte(idle ? LCD_CMD_IDMON : LCD_CMD_IDMOFF) < LCD_OK) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_setDisplayMode(unsigned char display) {
    lcd_status_t status;

    /* This command turns ON or OFF the display. */
    status = lcd_writeCommandByte(display ? LCD_CMD_DISPON : LCD_CMD_DISPOFF);
    if(status < LCD_OK) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_setDisplayInversion(unsigned char inversion) {
    lcd_status_t status;

    /* This command turns ON or OFF display inversion. */
    status = lcd_writeCommandByte(inversion ? LCD_CMD_INVON : LCD_CMD_INVOFF);
    if(status < LCD_OK) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_setGammaPredefined(unsigned char gamma) {
    /* check the input value; cannot be combined */
    switch(gamma) {
        /* correct arguments */
        case LCD_GAMMA_PREDEFINED_1: break;
        case LCD_GAMMA_PREDEFINED_2: break;
        case LCD_GAMMA_PREDEFINED_3: break;
        case LCD_GAMMA_PREDEFINED_4: break;

            /* incorrect argument */
        default: return LCD_FAIL;
    }

    /* Gamma Set */
    if(lcd_writeCommandByte(LCD_CMD_GAMSET) < LCD_OK) {
        return LCD_FAIL;
    }

    /* write argument */
    if(lcd_writeData(&gamma, sizeof(gamma)) < LCD_OK) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_setTearingEffectLine(unsigned char tearing) {
    /* check the input argument */
    if((tearing == LCD_TEARING_MODE_V) || (tearing == LCD_TEARING_MODE_VH)) {
        /* turn on tearing effect line */
        if(lcd_writeCommandByte(LCD_CMD_TEON) < LCD_OK) {
            return LCD_FAIL;
        }
        if(lcd_writeData(&tearing, sizeof(tearing)) < LCD_OK) {
            return LCD_FAIL;
        }
    } else {
        /* turn off tearing effect line */
        if(lcd_writeCommandByte(LCD_CMD_TEOFF) < LCD_OK) {
            return LCD_FAIL;
        }

        /* turning off does not require sending data about arguments */
    }

    return LCD_OK;
}

lcd_status_t lcd_setMemoryAccessControl(unsigned char flags) {
    unsigned char flags_copy = flags;

    /* Memory Access Control */
    if(lcd_writeCommandByte(LCD_CMD_MADCTL) < LCD_OK) {
        return LCD_FAIL;
    }

    /* write flags */
    if(lcd_writeData(&flags_copy, sizeof(flags_copy)) < LCD_OK) {
        return LCD_FAIL;
    }

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* if the new state of the MADCTL_MV flag is different than the previous */
    if((flags & LCD_MADCTL_MV) != lcd_settings->flag_madctl_mv) {
        unsigned short int temp;

        /* swap width with height */
        temp = lcd_settings->width;
        lcd_settings->width  = lcd_settings->height;
        lcd_settings->height = temp;

        /* swap width_offset with height_offset */
        temp = lcd_settings->width_offset;
        lcd_settings->width_offset  = lcd_settings->height_offset;
        lcd_settings->height_offset = temp;

        /* update flag_madctl_mv */
        lcd_settings->flag_madctl_mv = flags & LCD_MADCTL_MV;
    }

    return LCD_OK;
}

lcd_status_t lcd_setInterfacePixelFormat(unsigned char format) {
    unsigned char format_copy = format;

    /* check the input value; cannot be combined */
    switch(format) {
        /* correct arguments */
        case LCD_PIXEL_FORMAT_444: break;
        case LCD_PIXEL_FORMAT_565: break;
        case LCD_PIXEL_FORMAT_666: break;

            /* incorrect argument */
        default: return LCD_FAIL;
    }

    /* Interface Pixel Format Set */
    if(lcd_writeCommandByte(LCD_CMD_COLMOD) < LCD_OK) {
        return LCD_FAIL;
    }

    /* write argument */
    if(lcd_writeData(&format_copy, sizeof(format_copy)) < LCD_OK) {
        return LCD_FAIL;
    }

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* save interface pixel format for drawing functions */
    lcd_settings->interface_pixel_format = format;

    return LCD_OK;
}

lcd_status_t lcd_setWindowPosition(
        unsigned short int column_start,
        unsigned short int row_start,
        unsigned short int column_end,
        unsigned short int row_end
) {
    unsigned char buffer[4]; /* four bytes */

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* column ranges: (pdf v1.4 p105) (pdf v1.4 p128)
     * 'column_start' always must be less than or equal to 'column_end'.
     * When 'column_start' or 'column_end' are greater than maximum column
     * address, data of out of range will be ignored.
     *
     * 0 <= column_start <= column_end <= COLUMN_MAX
     */
    if(column_start > column_end) {
        return LCD_FAIL;
    }

    /* row ranges: (pdf v1.4 p105) (pdf v1.4 p130)
     * 'row_start' always must be less than or equal to 'row_end'.
     * When 'row_start' or 'row_end' are greater than maximum row address,
     * data of out of range will be ignored.
     *
     * 0 <= row_start <= row_end <= ROW_MAX
     */
    if(row_start > row_end) {
        return LCD_FAIL;
    }

    /* append offset to variables; after checking the ranges */
    /* width_offset  :: column :: X */
    /* height_offset :: row    :: Y */
    column_start += lcd_settings->width_offset;
    column_end   += lcd_settings->width_offset;
    row_start    += lcd_settings->height_offset;
    row_end      += lcd_settings->height_offset;

    /* write column address; requires 4 bytes of the buffer */
    buffer[0] = (column_start >> 8) & 0x00FF; /* MSB */ /* =0 for ST7735S */
    buffer[1] =  column_start       & 0x00FF; /* LSB */
    buffer[2] = (column_end   >> 8) & 0x00FF; /* MSB */ /* =0 for ST7735S */
    buffer[3] =  column_end         & 0x00FF; /* LSB */
    if(lcd_writeCommandByte(LCD_CMD_CASET) < LCD_OK) {
        return LCD_FAIL;
    }
    if(lcd_writeData(buffer, 4) < LCD_OK) {
        return LCD_FAIL;
    }

    /* write row address; requires 4 bytes of the buffer */
    buffer[0] = (row_start >> 8) & 0x00FF; /* MSB */ /* =0 for ST7735S */
    buffer[1] =  row_start       & 0x00FF; /* LSB */
    buffer[2] = (row_end   >> 8) & 0x00FF; /* MSB */ /* =0 for ST7735S */
    buffer[3] =  row_end         & 0x00FF; /* LSB */
    if(lcd_writeCommandByte(LCD_CMD_RASET) < LCD_OK) {
        return LCD_FAIL;
    }
    if(lcd_writeData(buffer, 4) < LCD_OK) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_activateMemoryWrite() {
    /* This command activate memory write. */
    if(lcd_writeCommandByte(LCD_CMD_RAMWR) < LCD_OK) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_drawPixel(
        unsigned short int x,
        unsigned short int y,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
) {
    unsigned char buffer[3]; /* three bytes */

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    if(lcd_setWindowPosition(x, y, x, y) < LCD_OK) {
        return LCD_FAIL;
    }

    if(lcd_activateMemoryWrite() < LCD_OK) {
        return LCD_FAIL;
    }

    /* prepare buffer and write data */
    switch(lcd_settings->interface_pixel_format) {
        /* case LCD_PIXEL_FORMAT_444 is not supported for this function */

        case LCD_PIXEL_FORMAT_565:
            buffer[0] = (red & 0xF8)  |  ((green >> 5) & 0x07);
            buffer[1] = ((green << 3) & 0xE0)  |  ((blue >> 3) & 0x1F);

            if(lcd_writeData(buffer, 2) < LCD_OK) {
                return LCD_FAIL;
            }
            break;

        case LCD_PIXEL_FORMAT_666:
            buffer[0] = red;
            buffer[1] = green;
            buffer[2] = blue;

            if(lcd_writeData(buffer, 3) < LCD_OK) {
                return LCD_FAIL;
            }
            break;

            /* unknown interface pixel format */
        default:
            return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_drawHorizontalLine(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int x1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
) {
    unsigned short int length;
    unsigned char buffer[3];      /* three bytes */
    unsigned char buffer_temp[3]; /* three bytes */
    unsigned char buffer_required_size = 0;

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* prepare buffer */
    switch(lcd_settings->interface_pixel_format) {
        /* case LCD_PIXEL_FORMAT_444 is not supported for this function */

        case LCD_PIXEL_FORMAT_565:
            buffer[0] = (red & 0xF8)  |  ((green >> 5) & 0x07);
            buffer[1] = ((green << 3) & 0xE0)  |  ((blue >> 3) & 0x1F);

            buffer_required_size = 2; /* two bytes required */
            break;

        case LCD_PIXEL_FORMAT_666:
            buffer[0] = red;
            buffer[1] = green;
            buffer[2] = blue;

            buffer_required_size = 3; /* three bytes required */
            break;

            /* unknown interface pixel format */
        default:
            return LCD_FAIL;
    }

    /* if(x0 > x1) then lcd_setWindowPosition will return LCD_FAIL */
    if(lcd_setWindowPosition(x0, y0, x1, y0) < LCD_OK) {
        return LCD_FAIL;
    }

    if(lcd_activateMemoryWrite() < LCD_OK) {
        return LCD_FAIL;
    }

    /* write data */
    for(length = x1 - x0 + 1; length > 0; length--) {
        /* copy buffer to buffer_temp */
        memcpy(buffer_temp, buffer, buffer_required_size);

        /* send a copy of the buffer; copy is overwritten with incoming data */
        if(lcd_writeData(buffer_temp, buffer_required_size) < LCD_OK) {
            return LCD_FAIL;
        }
    }

    return LCD_OK;
}

lcd_status_t lcd_drawVerticalLine(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int y1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
) {
    unsigned short int length;
    unsigned char buffer[3];      /* three bytes */
    unsigned char buffer_temp[3]; /* three bytes */
    unsigned char buffer_required_size = 0;

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* prepare buffer */
    switch(lcd_settings->interface_pixel_format) {
        /* case LCD_PIXEL_FORMAT_444 is not supported for this function */

        case LCD_PIXEL_FORMAT_565:
            buffer[0] = (red & 0xF8)  |  ((green >> 5) & 0x07);
            buffer[1] = ((green << 3) & 0xE0)  |  ((blue >> 3) & 0x1F);

            buffer_required_size = 2; /* two bytes required */
            break;

        case LCD_PIXEL_FORMAT_666:
            buffer[0] = red;
            buffer[1] = green;
            buffer[2] = blue;

            buffer_required_size = 3; /* three bytes required */
            break;

            /* unknown interface pixel format */
        default:
            return LCD_FAIL;
    }

    /* if(y0 > y1) then lcd_setWindowPosition will return LCD_FAIL */
    if(lcd_setWindowPosition(x0, y0, x0, y1) < LCD_OK) {
        return LCD_FAIL;
    }

    if(lcd_activateMemoryWrite() < LCD_OK) {
        return LCD_FAIL;
    }


    /* write data */
    for(length = y1 - y0 + 1; length > 0; length--) {
        /* copy buffer to buffer_temp */
        memcpy(buffer_temp, buffer, buffer_required_size);

        /* send a copy of the buffer; copy is overwritten with incoming data */
        if(lcd_writeData(buffer_temp, buffer_required_size) < LCD_OK) {
            return LCD_FAIL;
        }
    }

    return LCD_OK;
}

lcd_status_t lcd_drawRectangle(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int x1,
        unsigned short int y1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
) {
    /* see: documentation/notes/lcd_drawRectangle/lcd_drawRectangle.png */

    if(
            ((x1 - x0 + 1) >= 3) /* width  >= 3 */
            &&
            ((y1 - y0 + 1) >= 3) /* height >= 3 */
            ) {
        /* Horizontal 1 */
        if(lcd_drawHorizontalLine(x0, y0, x1, red, green, blue) < LCD_OK) {
            return LCD_FAIL;
        }

        /* Horizontal 2 */
        if(lcd_drawHorizontalLine(x0, y1, x1, red, green, blue) < LCD_OK) {
            return LCD_FAIL;
        }

        /* Vertical 1 */
        if(
                lcd_drawVerticalLine(x0, y0 + 1, y1 - 1, red, green, blue) < LCD_OK
                ) {
            return LCD_FAIL;
        }

        /* Vertical 2 */
        if(
                lcd_drawVerticalLine(x1, y0 + 1, y1 - 1, red, green, blue) < LCD_OK
                ) {
            return LCD_FAIL;
        }
    }

    /* draw other shapes */
    if(lcd_drawFilledRectangle(x0, y0, x1, y1, red, green, blue) < LCD_OK) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_drawFilledRectangle(
        unsigned short int x0,
        unsigned short int y0,
        unsigned short int x1,
        unsigned short int y1,
        unsigned char      red,
        unsigned char      green,
        unsigned char      blue
) {
    unsigned short int ix, iy; /* variables for iteration */
    unsigned short int width;
    unsigned short int height;
    unsigned char buffer[3];      /* three bytes */
    unsigned char buffer_temp[3]; /* three bytes */
    unsigned char buffer_required_size = 0;

    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* prepare buffer */
    switch(lcd_settings->interface_pixel_format) {
        /* case LCD_PIXEL_FORMAT_444 is not supported for this function */

        case LCD_PIXEL_FORMAT_565:
            buffer[0] = (red & 0xF8)  |  ((green >> 5) & 0x07);
            buffer[1] = ((green << 3) & 0xE0)  |  ((blue >> 3) & 0x1F);

            buffer_required_size = 2; /* two bytes required */
            break;

        case LCD_PIXEL_FORMAT_666:
            buffer[0] = red;
            buffer[1] = green;
            buffer[2] = blue;

            buffer_required_size = 3; /* three bytes required */
            break;

            /* unknown interface pixel format */
        default:
            return LCD_FAIL;
    }

    if(lcd_setWindowPosition(x0, y0, x1, y1) < LCD_OK) {
        return LCD_FAIL;
    }

    if(lcd_activateMemoryWrite() < LCD_OK) {
        return LCD_FAIL;
    }

    /* if(x0 > x1) or if(y0 > y1) then
     * lcd_setWindowPosition will return LCD_FAIL
     */
    width  = x1 - x0 + 1;
    height = y1 - y0 + 1;

    /* write data */
    for(iy = 0; iy < height; iy++) for(ix = 0; ix < width; ix++) {
            /* copy buffer to buffer_temp */
            memcpy(buffer_temp, buffer, buffer_required_size);

            /* send a copy of the buffer; copy is overwritten with incoming data */
            if(lcd_writeData(buffer_temp, buffer_required_size) < LCD_OK) {
                return LCD_FAIL;
            }
        }

    return LCD_OK;
}

lcd_status_t lcd_clearScreen(
        unsigned char red,
        unsigned char green,
        unsigned char blue
) {
    /* Prevent a bug of NULL pointer. 'lcd_settings' is required. */
    if(lcd_settings == NULL) {
        return LCD_FAIL;
    }

    /* clear screen */
    if(
            lcd_drawFilledRectangle(
                    0, 0, lcd_settings->width - 1, lcd_settings->height - 1,
                    red, green, blue
            ) < LCD_OK
            ) {
        return LCD_FAIL;
    }

    return LCD_OK;
}

lcd_status_t lcd_framebuffer_send(
        unsigned char * buffer,
        const    size_t length_buffer,
        const    size_t length_chunk
) {
    unsigned int i;
    unsigned int chunk_amount;
    unsigned int remainder;

    /* check inputs */
    if(buffer == NULL) {
        return LCD_FAIL;
    }
    if(length_buffer <= 0) {
        return LCD_FAIL;
    }
    if(length_chunk <= 0) {
        return LCD_FAIL;
    }

    chunk_amount = length_buffer / length_chunk; /* integer division */
    remainder    = length_buffer % length_chunk;

    /* send chunks */
    for(i = 0; i < chunk_amount; i++) {
        if(
                lcd_writeData(
                        buffer + i * length_chunk,
                        length_chunk
                ) < LCD_OK
                ) {
            return LCD_FAIL;
        }
    }

    /* send the remainder, if any */
    if(remainder > 0) {
        if(
                lcd_writeData(
                        buffer + chunk_amount * length_chunk,
                        remainder
                ) < LCD_OK
                ) {
            return LCD_FAIL;
        }
    }

    return LCD_OK;
}


/**
 * --- Above here is the generic driver code. This is where we incorporate the RP2040 spi functions.
*/


#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "pinout_rp2040.h"
#include "delay.h"

static bool dma_transfer_started = true;
static int dma_channel = -1;

static void wait_until_ready() {
    if (dma_transfer_started) {
        dma_channel_wait_for_finish_blocking(dma_channel);
        // Seems like the display requires some time between DMA transfers finishing and being properly ready. Hard to
        // tell precisely when we need this and when we don't, partial display writes in particular get
        // messed with
        dma_transfer_started = false;
        sleep_us(10);
    }
    while (spi_is_busy(spi0));
}

void lcd_delay(unsigned long int milliseconds) {
    sleep_ms(milliseconds);
}

void lcd_digitalWrite(unsigned short int pin, unsigned char value) {
    gpio_put(pin, value);
}

void lcd_spiWrite(unsigned char* buffer, size_t length) {

    if (!lcd_settings) {
        return;
    }

    bool command = true;
    if (gpio_get(lcd_settings->pin_communicationMode) == lcd_settings->dataMode_activeState) {
        command = false;
    }

    wait_until_ready();
    if (command) {
        spi_set_format(spi0, 8, 0, 0, SPI_MSB_FIRST);
        spi_write_blocking(spi0, buffer, length);
    } else {
        // data - pixel data is stored as uint16_ts that we need to send accordingly.
        // We'll need to assume the buffer is 16-bit values. Be careful about byte alignment and
        // sending in byte arrays for pixel data in user code.
        spi_set_format(spi0, 16, 0, 0, SPI_MSB_FIRST);
        if (length >= 16) {
            dma_channel_transfer_from_buffer_now(dma_channel, (uint16_t*)buffer, length/2);
            dma_transfer_started = true;
        } else {
            spi_write16_blocking(spi0, (uint16_t*)buffer, length/2);
        }
    }
}


/**
 * --- And this is where we map the provided driver functions to the badge's HAL.
 */

#include "display.h"

/** set important internal registers for the LCD display */
void display_init_device(void) {

    static lcd_t lcd = {
        .width = 128,
        .height = 160,
        // need to check these - as far as I can tell, they aren't published.
        // Framebuffer _should_ be large enough already and the old driver just wrote the extra columns/rows anyway.
        .width_offset = 0,
        .height_offset = 0,
        .pin_reset = BADGE_GPIO_DISPLAY_RESET,
        .interface_pixel_format = LCD_PIXEL_FORMAT_565,
        .dataMode_activeState = 1,
        .pin_communicationMode = BADGE_GPIO_DISPLAY_DC,
        .reset_activeState = 1,
        .flag_madctl_mv = FLAG_MADCTL_MV_DEFAULT,
    };

    lcd_setSettingsActive(&lcd);
    lcd_initialize();

}

/** set GPIO configuration for the LCD display */
void display_init_gpio(void) {

    gpio_init(BADGE_GPIO_DISPLAY_CS);
    gpio_set_function(BADGE_GPIO_DISPLAY_CS, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_DISPLAY_SCK);
    gpio_set_function(BADGE_GPIO_DISPLAY_SCK, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_DISPLAY_MOSI);
    gpio_set_function(BADGE_GPIO_DISPLAY_MOSI, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_DISPLAY_DC);
    gpio_set_dir(BADGE_GPIO_DISPLAY_DC, true);

    gpio_init(BADGE_GPIO_DISPLAY_RESET);
    gpio_set_dir(BADGE_GPIO_DISPLAY_RESET, true);

    spi_init(spi0, 15000000);

    if (dma_channel == -1) {
        dma_channel = dma_claim_unused_channel(true);
    }

    // DMA channel: use this for bulk data transfers, which are 16-bit
    dma_channel_config config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, spi_get_dreq(spi0, true));
    dma_channel_configure(dma_channel,  &config, &spi_get_hw(spi0)->dr, NULL, 0, false);

    display_init_device();
}

/** Perform init sequence on display */
void display_reset(void) {
    lcd_hardwareReset();
    lcd_softwareReset();
}
/** sets the current display region for calls to display_pixel() */
void display_rect(int x, int y, int width, int height) {
    lcd_setWindowPosition(x, y, width+x-1, height+y-1);
}
/** updates current pixel to the data in `pixel`. */
void display_pixel(unsigned short pixel) {
    // We want to use the display function that doesn't update the boundary rect
    lcd_writeData((uint8_t*)&pixel, 2);
}
/** Updates a consecutive sequence of pixels. */
void display_pixels(unsigned short *pixel, int number) {
    lcd_writeData((uint8_t*)pixel, number*2);
}
/** invert display */
static bool inverted = false;
static bool rotated = false;

static void update_madctl(void) {

    char flags = FLAG_MADCTL_MV_DEFAULT;

    if (inverted) {
        flags |= LCD_MADCTL_MX | LCD_MADCTL_MY;
    }

    if (rotated) {
        flags |= LCD_MADCTL_MV;
    }

    lcd_setMemoryAccessControl(flags);
}

void display_set_display_mode_inverted(void) {
    inverted = true;
    update_madctl();
}
/** uninvert display */
void display_set_display_mode_noninverted(void) {
    inverted = false;
    update_madctl();
}

/** get current display mode variable setting */
unsigned char display_get_display_mode(void) {
    return inverted ? DISPLAY_MODE_INVERTED : DISPLAY_MODE_NORMAL;
}

int display_get_rotation(void) {
    return rotated;
}
void display_set_rotation(int yes) {
    rotated = yes;
    update_madctl();
}
void display_color(unsigned short pixel) {

    const short display_width = 132;
    const short display_height = 162;

    display_rect(0, 0, display_width, display_height);
    for (int x=0; x<display_width; x++) {
        for (int y=0; y<display_height; y++) {
            display_pixel(pixel);
        }
    }
}

/** @brief Tell us if we're busy sending data to the display */
bool display_busy(void) {
    return spi_is_busy(spi0);
}