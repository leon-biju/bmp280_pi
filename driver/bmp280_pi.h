#ifndef __LEON_BMP280_PI
#define __LEON_BMP280_PI

#include <linux/regmap.h>
#include <linux/mutex.h>

struct bmp280_priv_state {
        struct regmap *regmap;
        struct mutex lock;
};

#endif