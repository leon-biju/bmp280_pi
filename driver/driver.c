// Just a dummy driver for now

#include <linux/iio/iio.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>

#define DRIVER_NAME "bme280_pi_dummy"

#define DUMMY_TEMP_RAW      25123   /* assuming scale of 0.001 so abt 25.1 C */
#define DUMMY_PRESSURE_RAW  101325  /* assuming scale of 0.001 so abt 1013.25 hPa */
#define DUMMY_HUMIDITY_RAW  55200   /* assuming scale of 0.001 so abt 55.2 % Rel Humidity */


enum bme280_pi_chan {
        CHAN_TEMP_IDX,
        CHAN_PRESSURE_IDX,
        CHAN_HUMIDITY_IDX
};

// Note: Since we are in kernel-space and cannot do floating point maths we 
//       just expose the raw value + scale and let userspace do the calculation
static const struct iio_chan_spec bme280_pi_channels[] = {
        {
                .type               = IIO_TEMP,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
                .scan_index         = CHAN_TEMP_IDX,
        },
        {
                .type               = IIO_PRESSURE,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
                .scan_index         = CHAN_PRESSURE_IDX,
        },
        {
                .type               = IIO_HUMIDITYRELATIVE,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
                .scan_index         = CHAN_HUMIDITY_IDX,
        }
};

/* For raw vals reads to just out_val with the raw value
   For scale values reads to out_val and out_val2 s.t. scale = out_val + out_val2(1e-6) */
static int bme280_pi_read_raw(
                struct iio_dev *indio_device,
                struct iio_chan_spec const *channel,
                int *out_val, int *out_val2, long mask)
{
        switch (mask) {
        case IIO_CHAN_INFO_RAW:
               switch (channel->type) {
                //Should take the actual reading here
                case IIO_TEMP:
                        *out_val = DUMMY_TEMP_RAW;
                        return IIO_VAL_INT;
                case IIO_PRESSURE:
                        *out_val = DUMMY_PRESSURE_RAW;
                        return IIO_VAL_INT;
                case IIO_HUMIDITYRELATIVE:
                        *out_val = DUMMY_HUMIDITY_RAW;
                        return IIO_VAL_INT;
                default:
                        return -EINVAL;
               }

        /* Scale factors so userspace can do value = raw * scale
           IIO_VAL_INT_PLUS_MICRO means scale = out_val + out_val2(1e-6) */
        case IIO_CHAN_INFO_SCALE:
                switch (channel->type) {
                case IIO_TEMP:
                        *out_val = 0;
                        *out_val2 = 1000; // 0.001 C
                        return IIO_VAL_INT_PLUS_MICRO;
                case IIO_PRESSURE:
                        *out_val = 0;
                        *out_val2 = 100; // 0.0001 kPa
                        return IIO_VAL_INT_PLUS_MICRO;
                case IIO_HUMIDITYRELATIVE:
                        *out_val = 0;
                        *out_val2 = 1000; // 0.001% RH
                        return IIO_VAL_INT_PLUS_MICRO;
                default:
                        return -EINVAL;
               }
        
        default:
               return -EINVAL;
       } 
}

/* Link the read raw function */
static const struct iio_info bme280_pi_info = {
        .read_raw = bme280_pi_read_raw,
};


static int bme280_pi_probe(struct platform_device *p_dev)
{
        struct iio_dev *indio_dev;

        indio_dev = devm_iio_device_alloc(&p_dev->dev, 0);
        if (!indio_dev) {
                return -ENOMEM;
        }

        indio_dev->name         = DRIVER_NAME;
        indio_dev->info         = &bme280_pi_info;
        indio_dev->modes        = INDIO_DIRECT_MODE;
        indio_dev->channels     = bme280_pi_channels;
        indio_dev->num_channels = ARRAY_SIZE(bme280_pi_channels);


        dev_info(&p_dev->dev, "bme280_pi: IIO dummy device registered\n");

        return devm_iio_device_register(&p_dev->dev, indio_dev);
}

static struct platform_driver bme280_pi_driver = {
        .probe = bme280_pi_probe,
        .driver = {
                .name = DRIVER_NAME
        }
};


/*
 * Since we do not have a real device yet :(
 * Let's create a fake device. 
 */

static struct platform_device *bme280_pi_pdev;

static int __init bme280_pi_init(void)
{
        int ret = platform_driver_register(&bme280_pi_driver);
        if (ret) {
                return ret;
        }

        bme280_pi_pdev = platform_device_register_simple(
                DRIVER_NAME,
                -1,
                NULL,
                0
        );

        if (IS_ERR(bme280_pi_pdev)) {
                platform_driver_unregister(&bme280_pi_driver);
                return PTR_ERR(bme280_pi_pdev);
        }
        pr_info("bme280_pi: Started driver\n");
        return 0;
}

static void __exit bme280_pi_exit(void)
{
        platform_device_unregister(bme280_pi_pdev);
        platform_driver_unregister(&bme280_pi_driver);

        pr_info("bme280_pi: Removed dummy device\n");
}

module_init(bme280_pi_init);
module_exit(bme280_pi_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("LEON BIJU");
MODULE_DESCRIPTION("BME280 Pi dummy IIO driver. Temp hardcoded channels, no hardware");
