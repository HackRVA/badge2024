/*!
 *  @file   color_sensor_cls16d2444_rp2040.c
 *  @author Peter Maxwell Warasila
 *  @date   April 2, 2024
 *
 *  @brief  RVASec Badge Color Sensor Driver Implementation - CLS-16D24-44
 *
 *------------------------------------------------------------------------------
 *
 *  Datasheet: https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/4352/CLS-16D24-44-DF8-TR8_Ver.5_9-7-20.pdf
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include <pico.h>
#include <pico/time.h>
#include <hardware/i2c.h>

#include "hardware/gpio.h"
#include "pinout_rp2040.h"
#include "color_sensor.h"

/*- Private Macro ------------------------------------------------------------*/
#define CLS_REG_SYSM_CTRL   (0x00)
#define CLS_SWRST           (1U << 7)
#define CLS_EN_WAIT         (1U << 6)
#define CLS_EN_IR           (1U << 1)
#define CLS_EN_CLS          (1U << 0)

#define CLS_REG_INT_CTRL    (0x01)
#define CLS_CLS_SYNC        (1U << 4)
#define CLS_EN_CINT         (1U << 0)

#define CLS_REG_INT_FLAG    (0x02)
#define CLS_INT_POR         (1U << 7)
#define CLS_DATA_FLAG       (1U << 6)
#define CLS_INT_CLS         (1U << 0)

#define CLS_REG_WAIT_TIME   (0x03)
#define CLS_WAIT_TIME_2_MS(WAIT_TIME)   ((WAIT_TIME) * 10)
#define CLS_WAIT_TIME_FROM_MS(MS)       ((MS) / 10)

#define CLS_REG_CLS_GAIN    (0x04)
#define CLS_DIODE_SELT      (1U << 7)
#define CLS_DIODE_SELT_X1   (0U << 7)
#define CLS_DIODE_SELT_X2   (1U << 7) /* default */

#define CLS_PGA_CLS         (0x1f)
#define CLS_PGA_CLS_X1      (0x01) /* default */
#define CLS_PGA_CLS_X4      (0x02)
#define CLS_PGA_CLS_X8      (0x04)
#define CLS_PGA_CLS_X32     (0x08)
#define CLS_PGA_CLS_X64     (0x10)

#define CLS_REG_CLS_TIME    (0x05)
#define CLS_CLSCONV_BITPOS  (4)
#define CLS_CLSCONV         (0xf << CLS_CLSCONV_BITPOS)
#define CLS_INT_TIME        (0x3)
#define CLS_INT_TIME_1      (0x0) /* 2.0667 ms */
#define CLS_INT_TIME_4      (0x1) /* 8.2668 ms */
#define CLS_INT_TIME_16     (0x2) /* 33.0672 ms */
#define CLS_INT_TIME_64     (0x3) /* 132.2688 ms - default */ 

#define CLS_REG_PERSISTENCE (0x0b)
#define CLS_PRS_CLS         (0xf)

#define CLS_REG_CLS_THRE_LL (0x0c)
#define CLS_REG_CLS_THRE_LH (0x0d)

#define CLS_REG_CLS_THRE_HL (0x0e)
#define CLS_REG_CLS_THRE_HH (0x0f)

#define CLS_REG_INT_SOURCE  (0x16)
#define CLS_INT_SRC         (0x1f)
#define CLS_INT_SRC_RCH     (0x01)
#define CLS_INT_SRC_GCH     (0x02)
#define CLS_INT_SRC_BCH     (0x04)
#define CLS_INT_SRC_WCH     (0x08) /* default */
#define CLS_INT_SRC_IRCH    (0x10)

#define CLS_REG_ERROR_FLAG  (0x17)
#define CLS_ERR_RCH         (1U << 0)
#define CLS_ERR_GCH         (1U << 1)
#define CLS_ERR_BCH         (1U << 2)
#define CLS_ERR_WCH         (1U << 3)
#define CLS_ERR_IRCH        (1U << 4)

#define CLS_REG_RCH_DATA_L  (0x1c)
#define CLS_REG_RCH_DATA_H  (0x1d)

#define CLS_REG_GCH_DATA_L  (0x1e)
#define CLS_REG_GCH_DATA_H  (0x1f)

#define CLS_REG_BCH_DATA_L  (0x20)
#define CLS_REG_BCH_DATA_H  (0x21)

#define CLS_REG_WCH_DATA_L  (0x22)
#define CLS_REG_WCH_DATA_H  (0x23)

#define CLS_REG_IRCH_DATA_L (0x24)
#define CLS_REG_IRCH_DATA_H (0x25)

#define CLS_REG_PROD_ID_L   (0xBC)
#define CLS_REG_PROD_ID_H   (0xBD)
#define CLS_PRODUCT_ID      (0x0712)

#define CLS_TRX_TIMEOUT_US  (1000) /* 1 ms */

/*- Private Variables --------------------------------------------------------*/
static struct color_sensor_context {
    i2c_inst_t *i2c;
    uint8_t addr;
    enum color_sensor_state state;
    enum color_sensor_error_code error_code;
    uint16_t product_id_read;
    struct color_sample last_sample;
} m_ctx;

/*- Private Methods ----------------------------------------------------------*/
static void set_error_location(struct color_sensor_context *ctx,
                               enum color_sensor_error_code location)
{
    ctx->error_code &= ~COLOR_SENSOR_ERROR_LOCATION;
    ctx->error_code |= (location & COLOR_SENSOR_ERROR_LOCATION);
}

static void set_i2c_error(struct color_sensor_context *ctx, int rc, 
                          int expected_length)
{
    enum color_sensor_error_code e = COLOR_SENSOR_ERROR_NONE;
    if (PICO_ERROR_GENERIC == rc) {
        e = COLOR_SENSOR_ERROR_I2C_NOADACK;
    } else if (PICO_ERROR_TIMEOUT == rc) {
        e = COLOR_SENSOR_ERROR_I2C_TIMEOUT;
    } else if (rc != expected_length) {
        e = COLOR_SENSOR_ERROR_I2C_DATALEN;
    }
    ctx->error_code |= e;
}

static int i2c_memrd(struct color_sensor_context *ctx, uint8_t reg,
                     void *dst, size_t dst_sz, uint32_t timeout_us)
{
    absolute_time_t t = make_timeout_time_us(timeout_us);
    int rc = i2c_write_blocking_until(ctx->i2c, ctx->addr,
                                      &reg, sizeof(reg),
                                      true, t);
    if ((rc < 0) || (rc != sizeof(reg))) {
        ctx->error_code |= COLOR_SENSOR_ERROR_MEMRD_WRITE;
        set_i2c_error(ctx, rc, sizeof(reg));
        return rc;
    }

    rc = i2c_read_timeout_us(ctx->i2c, ctx->addr,
                             dst, dst_sz,
                             false, t);
    if (rc != (int) dst_sz) {
        ctx->error_code |= COLOR_SENSOR_ERROR_MEMRD_READ;
        set_i2c_error(ctx, rc, dst_sz);
    }

    return rc;
}

static int i2c_regmodify(struct color_sensor_context *ctx, uint8_t reg, 
                         uint8_t mask, uint8_t value, uint32_t timeout_us)
{
    uint8_t data[2];
    data[0] = reg;

    absolute_time_t t = make_timeout_time_us(timeout_us);
    int rc = i2c_memrd(ctx, reg, &data[1], sizeof(data[1]), timeout_us);
    if (rc != sizeof(data[1])) {
        ctx->error_code |= COLOR_SENSOR_ERROR_RGMOD_MEMRD;
        set_i2c_error(ctx, rc, sizeof(data));
        return rc;
    }

    data[1] &= mask;
    data[1] |= value;

    rc = i2c_write_blocking_until(ctx->i2c, ctx->addr, 
                                  data, sizeof(data), false, t);
    if (rc != sizeof(data)) {
        ctx->error_code |= COLOR_SENSOR_ERROR_RGMOD_WRITE;
        set_i2c_error(ctx, rc, sizeof(data));
        return rc;
    }

    return 1;
}

static void color_sensor_up(struct color_sensor_context *ctx)
{
    /* Re-initialize context. */
    ctx->i2c = BADGE_I2C_COLOR_SENSOR;
    ctx->addr = BADGE_I2C_COLOR_SENSOR_ADDR;
    ctx->state = COLOR_SENSOR_STATE_NO_INIT;
    ctx->error_code = COLOR_SENSOR_ERROR_NONE;
    ctx->product_id_read = 0xffff;
    ctx->last_sample.error_flags = 0x1f;
    ctx->last_sample.rgbwi[COLOR_SAMPLE_INDEX_RED] = 0xffff;
    ctx->last_sample.rgbwi[COLOR_SAMPLE_INDEX_BLUE] = 0xffff;
    ctx->last_sample.rgbwi[COLOR_SAMPLE_INDEX_GREEN] = 0xffff;
    ctx->last_sample.rgbwi[COLOR_SAMPLE_INDEX_WHITE] = 0xffff;
    ctx->last_sample.rgbwi[COLOR_SAMPLE_INDEX_IR] = 0xffff;

    /* Ensure we have allowed the color sensor time to boot up. */
    sleep_until(from_us_since_boot(1 * 1000 * 1000 /* 1 sec */));

    /* Bring up the I2C controller. */
    i2c_init(m_ctx.i2c, BADGE_I2C_COLOR_SENSOR_BAUD);
    gpio_set_function(BADGE_GPIO_COLOR_SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_set_function(BADGE_GPIO_COLOR_SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_disable_pulls(BADGE_GPIO_COLOR_SENSOR_SCL);
    gpio_disable_pulls(BADGE_GPIO_COLOR_SENSOR_SDA);
    
    /* Check product ID register. */
    uint16_t product_id;
    int rc = i2c_memrd(ctx, CLS_REG_PROD_ID_L, 
                       &product_id, sizeof(product_id),
                       1 * 1000 * 1000 /* 1 sec for first read */);
    if ((rc < 0) || (rc != sizeof(product_id))
        || (CLS_PRODUCT_ID != product_id)) {
        set_error_location(ctx, COLOR_SENSOR_ERROR_UP_PID);
        ctx->state = COLOR_SENSOR_STATE_ERROR;
        return;
    }

    for (int retries = 1; retries > 0; retries--) {
        uint8_t int_flag;
        rc = i2c_memrd(ctx, CLS_REG_INT_FLAG, 
                       &int_flag, sizeof(int_flag),
                       CLS_TRX_TIMEOUT_US);
        if (rc != sizeof(int_flag)) {
            set_error_location(ctx, COLOR_SENSOR_ERROR_UP_IFLG);
            ctx->state = COLOR_SENSOR_STATE_ERROR;
            return;
        }

        if (int_flag & CLS_INT_POR) {
            /* we need to clear it */
            int_flag &= CLS_INT_POR;
            uint8_t tx_data[2];
            tx_data[0] = CLS_REG_INT_FLAG;
            tx_data[1] = int_flag;
            rc = i2c_write_timeout_us(ctx->i2c, ctx->addr,
                                      tx_data, sizeof(tx_data),
                                      false, CLS_TRX_TIMEOUT_US);
            if (rc != sizeof(tx_data)) {
                set_error_location(ctx, COLOR_SENSOR_ERROR_UP_POR);
                ctx->state = COLOR_SENSOR_STATE_ERROR;
                return;
            }

            /* Can exit the retry loop. */
            break;
        } else {
            /* Fuck, we need to reset this */
            if (retries == 0) {
                /* Fuuuuuuck we did that already. */
                set_error_location(ctx, COLOR_SENSOR_ERROR_UP_RTRY);
                ctx->state = COLOR_SENSOR_STATE_ERROR;
                return;
            }

            rc = i2c_regmodify(ctx, CLS_REG_SYSM_CTRL, 
                               (uint8_t) ~CLS_SWRST, CLS_SWRST,
                               CLS_TRX_TIMEOUT_US);
            if (rc != 1) {
                set_error_location(ctx, COLOR_SENSOR_ERROR_UP_RST);
                ctx->state = COLOR_SENSOR_STATE_ERROR;
                return;
            }
        }
    }

    /* Enable the color sensor and IR channel. */
    rc = i2c_regmodify(ctx, CLS_REG_SYSM_CTRL,
                       (uint8_t) ~(CLS_EN_IR | CLS_EN_CLS), 
                       CLS_EN_IR | CLS_EN_CLS,
                       CLS_TRX_TIMEOUT_US);
    if (rc != 1) {
        set_error_location(ctx, COLOR_SENSOR_ERROR_UP_EN);
        ctx->state = COLOR_SENSOR_STATE_ERROR;
        return;
    }

    /* Additional configuration goes here. */

    ctx->state = COLOR_SENSOR_STATE_READY;
}

static void color_sensor_down(struct color_sensor_context *ctx)
{
    int rc = i2c_regmodify(ctx, CLS_REG_SYSM_CTRL,
                           (uint8_t) ~CLS_EN_CLS, 0,
                           CLS_TRX_TIMEOUT_US);
    if (rc != 1) {
        set_error_location(ctx, COLOR_SENSOR_ERROR_DOWN);
        ctx->state = COLOR_SENSOR_STATE_ERROR;
    }
    else
    {
        ctx->state = COLOR_SENSOR_STATE_POWER_DOWN;
    }

    i2c_deinit(ctx->i2c);
}

/*- API ----------------------------------------------------------------------*/
void color_sensor_init(void)
{
    //color_sensor_up(&m_ctx); // defer to first use
}

enum color_sensor_state color_sensor_get_state(void)
{
    return m_ctx.state;
}

enum color_sensor_error_code color_sensor_get_error_code(void)
{
    return m_ctx.error_code;
}

enum color_sensor_state color_sensor_power_ctl(enum color_sensor_power_cmd cmd)
{
    enum color_sensor_state state = color_sensor_get_state();
    
    if ((COLOR_SENSOR_POWER_CMD_UP == cmd)
        && (COLOR_SENSOR_STATE_READY != state)) {
        /* Clear any errors and try again. */
        color_sensor_up(&m_ctx);
    } else if ((COLOR_SENSOR_POWER_CMD_DOWN == cmd)
               && (COLOR_SENSOR_STATE_POWER_DOWN != state)) {
        color_sensor_down(&m_ctx);
    }
    
    return color_sensor_get_state();
}

int color_sensor_get_sample(struct color_sample *sample)
{
    if (NULL == sample) {
        return PICO_ERROR_INVALID_ARG;
    } else if (m_ctx.state != COLOR_SENSOR_STATE_READY) {
	return PICO_ERROR_NOT_PERMITTED;
    }
    
    /* Read the errors. */
    int rc = i2c_memrd(&m_ctx, CLS_REG_ERROR_FLAG, 
                       &sample->error_flags, sizeof(sample->error_flags),
                       CLS_TRX_TIMEOUT_US);
    if (rc != sizeof(sample->error_flags))
    {
        set_error_location(&m_ctx, COLOR_SENSOR_ERROR_SAMP_EFLG);
        m_ctx.state = COLOR_SENSOR_STATE_ERROR;
        return rc;
    }

    rc = i2c_memrd(&m_ctx, CLS_REG_RCH_DATA_L,
                   sample->rgbwi, sizeof(sample->rgbwi),
		   CLS_TRX_TIMEOUT_US);
    if (rc != sizeof(sample->rgbwi))
    {
        set_error_location(&m_ctx, COLOR_SENSOR_ERROR_SAMP_READ);
        m_ctx.state = COLOR_SENSOR_STATE_ERROR;
        return rc;
    }

    m_ctx.last_sample = *sample;

    return 0;
}

