#ifndef __LEON_BMP280_PI
#define __LEON_BMP280_PI

#include <linux/regmap.h>
#include <linux/mutex.h>

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

#endif