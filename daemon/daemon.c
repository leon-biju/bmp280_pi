// SPDX-License-Identifier: MIT
#include "iio_reader.h"
#include "ring_buffer.h"

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

	struct reading sample;

	struct ring_buf buffer;

	if (ring_buf_init(&buffer, 12)) {
		fprintf(stderr, "Failed to initialise ring buffer!\n");
		return 1;
	}

	while (1) {
		if (iio_reader_read(&reader, &sample)) {
			fprintf(stderr, "read failed\n");
		} else {
			// add to buffer
			ring_buf_push(&buffer, &sample);

			// print out whats in the buffer
			printf("{");
			for (size_t i = 0; i < buffer.count; i++) {
				printf("%f, ", ring_buf_get(&buffer, i)->temp_c);
			}
			printf("}\n");
		}
		sleep(interval_s);
	}
	
	return 0;
}
