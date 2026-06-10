#pragma once

struct iio_reading {
	double temp_c;
	double pressure_kpa;
};

struct iio_reader {
	char temp_path[256];
	char pres_path[256];
};

int iio_reader_open(struct iio_reader *reader, const char *device_path);
int iio_reader_read(struct iio_reader *reader, struct iio_reading *out);
