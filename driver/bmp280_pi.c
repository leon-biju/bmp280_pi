#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>

#define DRIVER_NAME "bmp280_pi"

#define DUMMY_TEMP_RAW      25123
#define DUMMY_PRESSURE_RAW  101325

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
        struct iio_dev *indio_dev;
        indio_dev = devm_iio_device_alloc(&client->dev, 0);
        if (!indio_dev) {
                return -ENOMEM;
        }

        indio_dev->name = DRIVER_NAME;
        indio_dev->info = &bmp280_pi_info;
        indio_dev->modes = INDIO_DIRECT_MODE;
        indio_dev->channels = bmp280_pi_channels;
        indio_dev->num_channels = ARRAY_SIZE(bmp280_pi_channels);

        dev_info(&client->dev, "bmp280_pi: probed at 0x%02x\n", client->addr);
        
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
