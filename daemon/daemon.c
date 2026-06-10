#include "iio_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_DEVICE_PATH "/sys/bus/iio/devices/iio:device0"
#define DEFAULT_INTERVAL_S  2

int main(int argc, char *argv[])
{
	const char *device_path = DEFAULT_DEVICE_PATH;
	int interval_s = DEFAULT_INTERVAL_S;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'd' && i + 1 < argc)
			device_path = argv[++i];
		else if (argv[i][0] == '-' && argv[i][1] == 'i' && i + 1 < argc)
			interval_s = atoi(argv[++i]);
	}

	setvbuf(stdout, NULL, _IOLBF, 0);

	struct iio_reader reader;
	if (iio_reader_open(&reader, device_path)) {
		fprintf(stderr, "Failed to open IIO device at %s\n", device_path);
		return 1;
	}

	printf("polling %s every %ds\n", device_path, interval_s);

	struct iio_reading reading;
	while (1) {
		if (iio_reader_read(&reader, &reading)) {
			fprintf(stderr, "read failed\n");
		} else {
			printf("temp=%.2f°C  pressure=%.4f kPa\n", reading.temp_c, reading.pressure_kpa);
		}
		sleep(interval_s);
	}
	
	return 0;
}
