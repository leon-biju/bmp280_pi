// SPDX-License-Identifier: MIT
#pragma once

#include <time.h>

struct reading {
	double temp_c;
	double pressure_kpa;
	time_t timestamp;
};

struct iio_reader {
	char temp_path[256];
	char pres_path[256];
};

int iio_reader_open(struct iio_reader *reader, const char *device_path);
int iio_reader_read(struct iio_reader *reader, struct reading *out);
