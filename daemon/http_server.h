// SPDX-License-Identifier: MIT
#pragma once

#include <pthread.h>
#include <signal.h>

#include "iio_reader.h"
#include "stats.h"

/*
 * Snapshot of the latest sensor state SPSC using a standard mutex
 * Writer: polling thread
 * Reader: HTTP threads
 */
struct shared_state {
	pthread_mutex_t lock;
	struct reading       current;
	struct rolling_stats stats;
	int has_data; /* 0 until the first successful reading */
};

/*
 * Binds to port and serve requests until *running == 0 (set by signal handler in daemon.c)
 * Returns 0 on clean shutdown, -1 if the
 * listening socket could not be set up.
 */
int http_server_run(int port, struct shared_state *st, volatile sig_atomic_t *running);
