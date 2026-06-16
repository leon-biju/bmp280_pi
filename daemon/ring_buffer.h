// SPDX-License-Identifier: MIT
#pragma once

#include <stddef.h>
#include <time.h>

#include "iio_reader.h"

struct ring_buf {
	struct reading *samples;
	size_t capacity;
	size_t count;
	size_t head;
};

int                   ring_buf_init(struct ring_buf *rb, size_t capacity);
void                  ring_buf_free(struct ring_buf *rb);
void                  ring_buf_push(struct ring_buf *rb, const struct reading *sample);
const struct reading *ring_buf_get(const struct ring_buf *rb, size_t index);
