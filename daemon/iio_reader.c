// SPDX-License-Identifier: MIT
#include "iio_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int read_sysfs_double(const char *path, double *out)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    int ret = 0;
    if (fscanf(fp, "%lf", out) != 1)
        ret = -1;

    fclose(fp);
    return ret;
}

int iio_reader_open(struct iio_reader *reader, const char *device_path)
{
    snprintf(reader->temp_path, sizeof(reader->temp_path), "%s/in_temp_input", device_path);
    snprintf(reader->pres_path, sizeof(reader->pres_path), "%s/in_pressure_input", device_path);

    FILE *fp = fopen(reader->temp_path, "r");
    if (!fp) {
        return -1;
    }
    fclose(fp);

    return 0;
}

int iio_reader_read(struct iio_reader *reader, struct iio_reading *out)
{
    double temp_raw;
    if (read_sysfs_double(reader->temp_path, &temp_raw))
        return -1;

    if (read_sysfs_double(reader->pres_path, &out->pressure_kpa))
        return -1;

    /* Driver returns millidegrees C as per IIO standard */
    out->temp_c = temp_raw / 1000.0;

    return 0;
}
