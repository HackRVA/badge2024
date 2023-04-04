
#include <pico/stdlib.h>
#include <hardware/spi.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <pinout_rp2040.h>
#include <delay.h>
#include <display.h>
#include <st7735s.h>

/*- ST7735S Driver Glue ------------------------------------------------------*/
static bool dma_transfer_started = true;
static int dma_channel = -1;
static bool writing_pixels;

static void wait_until_ready() {
    if (dma_transfer_started) {
        dma_channel_wait_for_finish_blocking(dma_channel);
        dma_transfer_started = false;
    }
    while (spi_is_busy(BADGE_SPI_DISPLAY));
}

void lcd_delay(unsigned long int milliseconds) {
    sleep_ms(milliseconds);
}

void lcd_digitalWrite(unsigned short int pin, unsigned char value) {
    gpio_put(pin, value);
}

void lcd_spiWrite(unsigned char* buffer, size_t length) {
    wait_until_ready();

    if (writing_pixels)
    {
        /* 
         *  We always respect the LCD API's length as 'bytes' to write, but the
         *  DMA API expects the number of transfers (in our case, pixels).
         */
        length /= 2;

        dma_transfer_started = true;
        spi_set_format(BADGE_SPI_DISPLAY, 16, 0, 0, SPI_MSB_FIRST);
        dma_channel_transfer_from_buffer_now(dma_channel, buffer, length);
    }
    else
    {
        spi_set_format(BADGE_SPI_DISPLAY, 8, 0, 0, SPI_MSB_FIRST);
        spi_write_blocking(BADGE_SPI_DISPLAY, buffer, length);
    }
}

/** set important internal registers for the LCD display */
void display_init_device(void) {

    static lcd_t lcd = {
        .width = 128,
        .height = 160,
        .width_offset = 0,
        .height_offset = 0,
        .pin_reset = BADGE_GPIO_DISPLAY_RESET,
        .interface_pixel_format = LCD_PIXEL_FORMAT_565,
        .dataMode_activeState = 1,
        .pin_communicationMode = BADGE_GPIO_DISPLAY_DC,
        .reset_activeState = 0,
    };

    lcd_setSettingsActive(&lcd);
    lcd_hardwareReset();
    lcd_initialize();
    lcd_setSleepMode(LCD_SLEEP_OUT);
    lcd_setMemoryAccessControl(LCD_MADCTL_BGR);
    lcd_setInterfacePixelFormat(LCD_PIXEL_FORMAT_565);
    lcd_setGammaPredefined(LCD_GAMMA_PREDEFINED_3);
    lcd_setDisplayInversion(LCD_INVERSION_OFF);
    lcd_setTearingEffectLine(LCD_TEARING_OFF);
    lcd_setDisplayMode(LCD_DISPLAY_ON);
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

    spi_init(BADGE_SPI_DISPLAY, 15000000);

    if (dma_channel == -1) {
        dma_channel = dma_claim_unused_channel(true);
    }

    // DMA channel: use this for bulk data transfers, which are 16-bit
    dma_channel_config config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, spi_get_dreq(BADGE_SPI_DISPLAY, true));
    dma_channel_configure(dma_channel,  &config, &spi_get_hw(BADGE_SPI_DISPLAY)->dr, NULL, 0, false);
}

/** Perform init sequence on display */
void display_reset(void) {
    lcd_hardwareReset();
    lcd_softwareReset();

    display_init_device();
}
/** sets the current display region for calls to display_pixel() */
void display_rect(int x, int y, int width, int height) {
    lcd_setWindowPosition(x, y, width+x, height+y);
    lcd_activateMemoryWrite();
}
/** updates current pixel to the data in `pixel`. */
void display_pixel(unsigned short pixel) {
    display_pixels(&pixel, 1);
}
/** Updates a consecutive sequence of pixels. */
void display_pixels(unsigned short *pixel, int number) {
    writing_pixels = true;
    /* Always respect the LCD API's length as 'bytes' to write*/
    lcd_writeData((uint8_t*)pixel, number * 2);
    writing_pixels = false;
}
/** invert display */
static bool inverted = false;
static bool rotated = false;

static void update_madctl(void) {

    char flags = LCD_MADCTL_BGR;

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
    return spi_is_busy(BADGE_SPI_DISPLAY);
}