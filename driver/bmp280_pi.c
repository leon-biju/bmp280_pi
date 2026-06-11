// SPDX-License-Identifier: GPL-2.0-only
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/regmap.h>
#include <linux/mutex.h>

#define DRIVER_NAME "bmp280_pi"


// Note: Please see the official Bosch BMP280 datasheet for setting these values

/* Register Addresses */
#define BMP280_REG_READINGS      0xF7
#define BMP280_READINGS_LEN      6
#define BMP280_REG_CHIP_ID       0xD0
#define BMP280_REG_CONFIG        0xF5
#define BMP280_REG_CTRL_MEAS     0xF4
#define BMP280_REG_CALIB_START   0x88
#define BMP280_CALIB_LEN         24

#define BMP280_CHIP_ID           0x58


/* Config settings */
#define BMP280_STANDBY_SETTING   0b010 // 125ms standby
#define BMP280_FILTER_SETTING    0b100 // filter coefficient 16
#define BMP280_SPI3W_SETTING     0b0   // off since we use I2C

/* Control settings */
#define BMP280_MODE_CONTROL      0b11  // Normal mode
#define BMP280_PRES_OS_RATE      0b011 // corresponds to x4
#define BMP280_TEMP_OS_RATE      0b011 // corresponds to x4


struct bmp280_priv_state {
        struct regmap *regmap;
        struct mutex lock;

        struct bmp280_calib_coeffs {
                // Temperature coefficients
                u16 dig_T1;
                s16 dig_T2;
                s16 dig_T3;

                // Pressure coefficients omd
                u16 dig_P1;
                s16 dig_P2;
                s16 dig_P3;
                s16 dig_P4;
                s16 dig_P5;
                s16 dig_P6;
                s16 dig_P7;
                s16 dig_P8;
                s16 dig_P9;

        } parameter_buffer;
};

static const struct iio_chan_spec bmp280_pi_channels[] = {
        {
                .type               = IIO_TEMP,
                .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
        },
        {
                .type               = IIO_PRESSURE,
                .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
        },
};


// These two compensation functions are ripped straight from the Bosch BMP280 datasheet
static s32 bmp280_pi_compensate_temp(s32 adc_T, struct bmp280_calib_coeffs *calib, s32 *out_t_fine)
{
        s32 var1, var2, T;

        var1 = ((((adc_T >> 3) - ((s32)calib->dig_T1 << 1))) * ((s32)calib->dig_T2)) >> 11;
        var2 = (((((adc_T >> 4) - ((s32)calib->dig_T1)) * ((adc_T >> 4) - ((s32)calib->dig_T1))) >> 12) * ((s32)calib->dig_T3)) >> 14;

        *out_t_fine = var1 + var2;
        T = (*out_t_fine * 5 + 128) >> 8;

        return T;
}

static u32 bmp280_pi_compensate_pres(s32 adc_P, struct bmp280_calib_coeffs *calib, s32 t_fine)
{
        s64 var1, var2, P;

        var1 = ((s64)t_fine) - 128000;
        var2 = var1 * var1 * (s64)calib->dig_P6;
        var2 = var2 + ((var1 * (s64)calib->dig_P5) << 17);
        var2 = var2 + (((s64)calib->dig_P4) << 35);
        var1 = ((var1 * var1 * (s64)calib->dig_P3) >> 8) +
               ((var1 * (s64)calib->dig_P2) << 12);
        var1 = ((((s64)1) << 47) + var1) * ((s64)calib->dig_P1) >> 33;

        if (var1 == 0)
                return 0;

        P = ((((s64)1048576 - adc_P) << 31) - var2) * 3125;
        P = P / var1;
        var1 = (((s64)calib->dig_P9) * (P >> 13) * (P >> 13)) >> 25;
        var2 = (((s64)calib->dig_P8) * P) >> 19;
        P = ((P + var1 + var2) >> 8) + (((s64)calib->dig_P7) << 4);

        return (u32)P;
}

static int bmp280_pi_read_raw(
                struct iio_dev *indio_dev,
                struct iio_chan_spec const *channel,
                int *out_val, int *out_val2, long mask)
{
        int ret;

        struct bmp280_priv_state *priv_state = iio_priv(indio_dev);
        struct regmap *regmap = priv_state->regmap;

        mutex_lock(&priv_state->lock);

        // 1. Take the readings in one fell swoop
        struct {
                u8 pres_msb;
                u8 pres_lsb;
                u8 pres_xlsb;

                u8 temp_msb;
                u8 temp_lsb;
                u8 temp_xlsb;
        } r;

        ret = regmap_bulk_read(regmap, BMP280_REG_READINGS, &r, BMP280_READINGS_LEN);
        if (ret) {
                dev_err(&indio_dev->dev, "Failed to take readings at register: 0x%02X\n", BMP280_REG_READINGS);
                return ret;
        }

        // 2. Assemble readings proper
        s32 temp_adc = (r.temp_msb << 12) | (r.temp_lsb << 4) | (r.temp_xlsb >> 4);
        s32 pres_adc = (r.pres_msb << 12) | (r.pres_lsb << 4) | (r.pres_xlsb >> 4);

        s32 t_fine;
        s32 T = bmp280_pi_compensate_temp(temp_adc, &priv_state->parameter_buffer, &t_fine);

        s32 P = bmp280_pi_compensate_pres(pres_adc, &priv_state->parameter_buffer, t_fine);

        switch (mask) {
        case IIO_CHAN_INFO_PROCESSED:
                switch (channel->type) {
                case IIO_TEMP:
                        // returns in millidegrees C
                        *out_val = T * 10; 
                        ret = IIO_VAL_INT;
                        break;
                case IIO_PRESSURE:
                        // returns in kPa
                        *out_val = P;
                        *out_val2 = 256 * 1000;
                        ret = IIO_VAL_FRACTIONAL;
                        break;
                default:
                        ret = -EINVAL;
                        break;
                }
                break;
        default:
                ret = -EINVAL;
                break;
        }

        mutex_unlock(&priv_state->lock);

        return ret;
}

static const struct iio_info bmp280_pi_info = {
        .read_raw = bmp280_pi_read_raw
};

static int bmp280_pi_probe(struct i2c_client *client)
{
        int ret;

        // 0. Allocate space
        struct iio_dev *indio_dev;
        indio_dev = devm_iio_device_alloc(&client->dev, sizeof(struct bmp280_priv_state));
        if (!indio_dev) {
                return -ENOMEM;
        }

        // 1. Setup driver's private state
        struct bmp280_priv_state *priv_state = iio_priv(indio_dev);

        static const struct regmap_config cfg = {
                .reg_bits = 8,
                .val_bits = 8,
        };
        struct regmap *regmap = devm_regmap_init_i2c(client, &cfg);

        if (IS_ERR(regmap)) {
                return PTR_ERR(regmap);
        }

        priv_state->regmap = regmap;

        mutex_init(&priv_state->lock);

        // 2. Chip ID check for BMP280
        unsigned int chip_id;

        ret = regmap_read(regmap, BMP280_REG_CHIP_ID, &chip_id);
        if (ret) {
                dev_err(&client->dev, "Failed to read Chip ID at register: 0x%02X\n", BMP280_REG_CHIP_ID);
                return ret;
        }
        if (chip_id != BMP280_CHIP_ID) {
                dev_err(&client->dev, "Expected chip ID 0x%02X, but got 0x%02X\n", BMP280_CHIP_ID, chip_id);
                return -ENODEV;
        }

        // 3. Read calibration values into priv_state

        // Note: we don't need to worry about endianness since we set val_bits = 8
        // Note: make sure the struct we are reading into is packed tight with no padding.
        ret = regmap_bulk_read(regmap, BMP280_REG_CALIB_START, &priv_state->parameter_buffer, BMP280_CALIB_LEN);
        if (ret) {
                dev_err(&client->dev, "Failed to read calibration parameters at register: 0x%02X\n", BMP280_REG_CALIB_START);
                return ret;
        }

        // 4. Set config variable
        u8 config = (BMP280_STANDBY_SETTING << 5) | (BMP280_FILTER_SETTING << 2) | BMP280_SPI3W_SETTING;
        ret = regmap_write(regmap, BMP280_REG_CONFIG, config);
        if (ret) {
                dev_err(&client->dev, "Failed to write to config at register: 0x%02X\n", BMP280_REG_CONFIG);
                return ret;
        }

        // 5. Set control variable
        u8 ctrl_meas = (BMP280_TEMP_OS_RATE << 5) | (BMP280_PRES_OS_RATE << 2) | BMP280_MODE_CONTROL;
        ret = regmap_write(regmap, BMP280_REG_CTRL_MEAS, ctrl_meas);
        if (ret) {
                dev_err(&client->dev, "Failed to write to ctrl_meas at register: 0x%02X\n", BMP280_REG_CTRL_MEAS);
                return ret;
        }
        // 6. Fill in and register the device
        indio_dev->name = DRIVER_NAME;
        indio_dev->info = &bmp280_pi_info;
        indio_dev->modes = INDIO_DIRECT_MODE;
        indio_dev->channels = bmp280_pi_channels;
        indio_dev->num_channels = ARRAY_SIZE(bmp280_pi_channels);

        dev_info(&client->dev, "Successfully probed at 0x%02X\n", client->addr);
        
        return devm_iio_device_register(&client->dev, indio_dev);
}


static const struct of_device_id bmp280_pi_of_match[] = {
        {.compatible = "leon,bmp280-pi"},
        {} //null terminator here so kernel doesn't walk off the edge
};

MODULE_DEVICE_TABLE(of, bmp280_pi_of_match);

static struct i2c_driver bmp280_pi_driver = {
        .driver = {
                .name = DRIVER_NAME,
                .of_match_table = bmp280_pi_of_match,
        },
        .probe = bmp280_pi_probe,
};
module_i2c_driver(bmp280_pi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LEON BIJU");
MODULE_DESCRIPTION("BMP280 Pi IIO driver. Exposes temperature in m°C and pressure in kPa.");
