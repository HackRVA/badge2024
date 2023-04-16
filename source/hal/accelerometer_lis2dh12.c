/**
 *  @file   accelerometer_lis2dh12.c
 *  @author Peter Maxwell Warasila
 *  @date   April 13, 2023
 *
 *  @brief  RVASec Badge accelerometer driver for RP2040 and LIS2DH
 */

#include <stdint.h>
#include <stddef.h>

#include <pinout_rp2040.h>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/dma.h>

#include <utils.h>

#include <accelerometer.h>

uint8_t whoami;
size_t sample;
union acceleration samples[32];

static int read(uint8_t addr, bool multiple, uint8_t *dst, size_t len)
{
    uint8_t cmd = 0x80 | (multiple ? 0x40 : 0x00) | (addr & 0x3f);
    gpio_put(BADGE_GPIO_ACCEL_CS, false);
    spi_write_blocking(BADGE_SPI_ACCEL, &cmd, sizeof(cmd));
    int br = spi_read_blocking(BADGE_SPI_ACCEL, 0, dst, len);
    gpio_put(BADGE_GPIO_ACCEL_CS, true);
    return br;
}

static int write(uint8_t addr, bool multiple, const uint8_t *src, size_t len)
{
    uint8_t cmd = (multiple ? 0x40 : 0x00) | (addr & 0x3f);
    gpio_put(BADGE_GPIO_ACCEL_CS, false);
    spi_write_blocking(BADGE_SPI_ACCEL, &cmd, sizeof(cmd));
    int bw = spi_write_blocking(BADGE_SPI_ACCEL, src, len);
    gpio_put(BADGE_GPIO_ACCEL_CS, true);
    return bw;
}

static mG_t raw2mG(int16_t raw)
{
    mG_t mG;

    /*
     * In normal mode, the LIS2DH12 gives left-justified 10-bit signed ints.
     * This translates to a division by 64. However, since the LIS2DH12 always
     * works on a 12-bit ADC value internally, we can always get the correctly
     * scaled value by assuming the value is a 12-bit value.
     */
    raw /= 16; /* Divides preserves sign */

    mG = raw;

    return mG;
}

static void int1_cb(uint gpio, uint32_t event_mask)
{
    (void) gpio;
    (void) event_mask;

    size_t next_sample = (sample + 1) % ARRAY_SIZE(samples);

    int16_t raw[3];
    int br = read(0x28, true, (uint8_t *) raw, sizeof(raw));
    if (br != sizeof(raw)) {
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(raw); i++) {
        samples[next_sample].a[i] = raw2mG(raw[i]); /* 4 mG / LSB in normal mode */
    }

    sample = next_sample;
}

void accelerometer_init_gpio()
{
    /* Pull interrupt line high */
    gpio_init(BADGE_GPIO_ACCEL_INT1);
    gpio_set_dir(BADGE_GPIO_ACCEL_INT1, false);
    gpio_set_pulls(BADGE_GPIO_ACCEL_INT1, true, false);

    /* Enable interrupt line */
    // gpio_set_irq_enabled_with_callback(BADGE_GPIO_ACCEL_INT1,
    //                                    GPIO_IRQ_EDGE_FALL, true, int1_cb);

    /* SPI line initialization */
    gpio_init(BADGE_GPIO_ACCEL_CS);
    gpio_set_dir(BADGE_GPIO_ACCEL_CS, true);
    gpio_put(BADGE_GPIO_ACCEL_CS, true);
    //gpio_set_function(BADGE_GPIO_ACCEL_CS, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_ACCEL_SCK);
    gpio_set_function(BADGE_GPIO_ACCEL_SCK, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_ACCEL_MOSI);
    gpio_set_function(BADGE_GPIO_ACCEL_MOSI, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_ACCEL_MISO);
    gpio_set_function(BADGE_GPIO_ACCEL_MISO, GPIO_FUNC_SPI);

    /* 1 MHz is within capabilities per ST DocID025056 Rev 6 section 2.4.1 */
    spi_init(BADGE_SPI_ACCEL, 1000000);

    /*
     * Using ST DocID025056 Rev 6 section 6.2.1 and RP2040 Datasheet section
     * 4.4.3.13, setting the polarity inversion and half-cycle phase offset bits
     * configures the RP2040 SPI peripheral to match the protocol requirements
     */
    spi_set_format(BADGE_SPI_ACCEL, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
}

void accelerometer_init()
{
    int br = read(0x0f, false, &whoami, sizeof(whoami));

    if ((br != sizeof(whoami)) || (whoami != 0x33)) {
        return;
    }

    static const uint8_t ctrl_regs[] = {
        0x47,   /* CTRL_REG1 (20h) */
        0x00,   /* CTRL_REG2 (21h) */
        0x00,   /* CTRL_REG3 (22h) */
        0x80,   /* CTRL_REG4 (23h) */
        0x00,   /* CTRL_REG5 (24h) */
        0x00,   /* CTRL_REG6 (25h) */
    };

    write(0x20, true, ctrl_regs, sizeof(ctrl_regs));
}

uint8_t accelerometer_whoami()
{
    return whoami;
}

union acceleration accelerometer_last_sample()
{
    if (whoami == 0x33) {
        int1_cb(0, 0);
    }

    return samples[sample];
}
