#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/regmap.h>
#include <linux/mutex.h>

#define DRIVER_NAME "bmp280_pi"

#define DUMMY_TEMP_RAW      25123
#define DUMMY_PRESSURE_RAW  101325

struct bmp280_priv_state {
        struct regmap *regmap;
        struct mutex lock;

        struct {
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
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
        },
        {
                .type               = IIO_PRESSURE,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
        },
};

/* For raw vals reads to just out_val with the raw value
   For scale values reads to out_val and out_val2 s.t. scale = out_val + out_val2(1e-6) */
static int bmp280_pi_read_raw(
                struct iio_dev *indio_device,
                struct iio_chan_spec const *channel,
                int *out_val, int *out_val2, long mask)
{
        switch (mask) {
        case IIO_CHAN_INFO_RAW:
                switch (channel->type) {
                case IIO_TEMP:
                        *out_val = DUMMY_TEMP_RAW;
                        return IIO_VAL_INT;
                case IIO_PRESSURE:
                        *out_val = DUMMY_PRESSURE_RAW;
                        return IIO_VAL_INT;
                default:
                        return -EINVAL;
                }
        case IIO_CHAN_INFO_SCALE:
                switch (channel->type) {
                case IIO_TEMP:
                        *out_val = 0;
                        *out_val2 = 1000;
                        return IIO_VAL_INT_PLUS_MICRO;
                case IIO_PRESSURE:
                        *out_val = 0;
                        *out_val2 = 100;
                        return IIO_VAL_INT_PLUS_MICRO;
                default:
                        return -EINVAL;
                }
        default:
                return -EINVAL;        
        }
}

static const struct iio_info bmp280_pi_info = {
        .read_raw = bmp280_pi_read_raw
};

static int bmp280_pi_probe(struct i2c_client *client)
{
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

        // 2. Chip ID check for BMP280 so register 0xD0 reads 0x58
        unsigned int reg_val;

        int res = regmap_read(regmap, 0xD0, &reg_val);
        if (res) {
                dev_err(&client->dev, "Failed to read regmap!");
                return res;
        }
        if (reg_val != 0x58) {
                dev_err(&client->dev, "Register 0xD0: expected 0x58 but got 0x%02X\n", reg_val);
                return -ENODEV;
        }

        // 3. Read calibration values into priv_state
        // Note: we don't need to worry about endianness since we set val_bits = 8
        // Note: make sure the struct is packed tight with no padding.
        regmap_bulk_read(regmap, 0x88, &priv_state->parameter_buffer, 24);

        // Sanity print
        dev_info(&client->dev, "dig_T1=%d", priv_state->parameter_buffer.dig_T1);
        dev_info(&client->dev, "dig_P1=%d", priv_state->parameter_buffer.dig_P1);
        dev_info(&client->dev, "dig_T2=%d", priv_state->parameter_buffer.dig_T2);
        dev_info(&client->dev, "dig_P2=%d", priv_state->parameter_buffer.dig_P2);

        // 4. Fill in and register the device
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
MODULE_DESCRIPTION("BMP280 Pi dummy IIO driver. Temp hardcoded channels. Registers real device");
