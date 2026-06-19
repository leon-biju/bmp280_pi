// SPDX-License-Identifier: MIT
#define _POSIX_C_SOURCE 200809L

#include "http_server.h"
#include "iio_reader.h"
#include "ring_buffer.h"
#include "stats.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_DEVICE_PATH "/sys/bus/iio/devices/iio:device0"
#define DEFAULT_INTERVAL_S  2
#define DEFAULT_PORT        8080

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig)
{
	(void)sig;
	running = 0;
}

struct poll_args {
	struct iio_reader   *reader;
	struct ring_buf     *buffer;
	int                  interval_s;
	struct shared_state *st;
};

/*
 * Background thread: read the sensor, update the ring buffer and rolling stats,
 * and publish the result into the shared snapshot for the HTTP threads.
 */
static void *poll_loop(void *arg)
{
	struct poll_args *pa = arg;

	/* Keep SIGTERM/SIGINT on the main thread so accept() gets interrupted. */
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	struct reading sample;

	while (running) {
		if (iio_reader_read(pa->reader, &sample)) {
			fprintf(stderr, "read failed\n");
		} else {
			ring_buf_push(pa->buffer, &sample);

			struct rolling_stats stats;
			if (rolling_stats_compute(pa->buffer, &stats) == 0) {
				pthread_mutex_lock(&pa->st->lock);
				pa->st->current  = sample;
				pa->st->stats    = stats;
				pa->st->has_data = 1;
				pthread_mutex_unlock(&pa->st->lock);
			}
		}
		sleep(pa->interval_s);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	const char *device_path = DEFAULT_DEVICE_PATH;
	int interval_s = DEFAULT_INTERVAL_S;
	int port = DEFAULT_PORT;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'd' && i + 1 < argc)
			device_path = argv[++i];
		else if (argv[i][0] == '-' && argv[i][1] == 'i' && i + 1 < argc)
			interval_s = atoi(argv[++i]);
		else if (argv[i][0] == '-' && argv[i][1] == 'p' && i + 1 < argc)
			port = atoi(argv[++i]);
	}

	setvbuf(stdout, NULL, _IOLBF, 0);

	// systemd sends SIGTERM on systemctl stop
	struct sigaction sa = { .sa_handler = handle_signal };
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	// A client that disconnects mid-write must not kill the daemon.
	signal(SIGPIPE, SIG_IGN);

	struct iio_reader reader;
	if (iio_reader_open(&reader, device_path)) {
		fprintf(stderr, "Failed to open IIO device at %s\n", device_path);
		return 1;
	}

	printf("polling %s every %ds\n", device_path, interval_s);

	struct ring_buf buffer;
	if (ring_buf_init(&buffer, 12)) {
		fprintf(stderr, "Failed to initialise ring buffer!\n");
		return 1;
	}

	struct shared_state state = { .lock = PTHREAD_MUTEX_INITIALIZER };

	struct poll_args pa = {
		.reader     = &reader,
		.buffer     = &buffer,
		.interval_s = interval_s,
		.st         = &state,
	};

	pthread_t poller;
	if (pthread_create(&poller, NULL, poll_loop, &pa)) {
		fprintf(stderr, "Failed to start polling thread\n");
		ring_buf_free(&buffer);
		return 1;
	}

	http_server_run(port, &state, &running);

	printf("Shutting down daemon!\n");
	pthread_join(poller, NULL);
	ring_buf_free(&buffer);
	return 0;
}
