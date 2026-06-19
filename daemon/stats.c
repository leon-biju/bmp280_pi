// SPDX-License-Identifier: MIT
#include "stats.h"

#include <float.h>
#include <string.h>

int rolling_stats_compute(const struct ring_buf *rb, struct rolling_stats *out)
{
	memset(out, 0, sizeof(*out));

	if (rb->count == 0)
		return -1;

	double temp_sum = 0.0;
	double pres_sum = 0.0;
	double temp_min = DBL_MAX, temp_max = -DBL_MAX;
	double pres_min = DBL_MAX, pres_max = -DBL_MAX;

	for (size_t i = 0; i < rb->count; i++) {
		const struct reading *s = ring_buf_get(rb, i);

		temp_sum += s->temp_c;
		pres_sum += s->pressure_kpa;

		if (s->temp_c < temp_min)  temp_min = s->temp_c;
		if (s->temp_c > temp_max)  temp_max = s->temp_c;
		if (s->pressure_kpa < pres_min) pres_min = s->pressure_kpa;
		if (s->pressure_kpa > pres_max) pres_max = s->pressure_kpa;
	}

	out->temp.mean = temp_sum / rb->count;
	out->temp.min  = temp_min;
	out->temp.max  = temp_max;

	out->pressure.mean = pres_sum / rb->count;
	out->pressure.min  = pres_min;
	out->pressure.max  = pres_max;

	out->sample_count = rb->count;

	return 0;
}
