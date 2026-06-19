// SPDX-License-Identifier: MIT
#pragma once

#include "ring_buffer.h"

struct stats {
	double mean;
	double min;
	double max;
};

struct rolling_stats {
	struct stats temp;
	struct stats pressure;
	size_t sample_count;
};

int rolling_stats_compute(const struct ring_buf *rb, struct rolling_stats *out);
