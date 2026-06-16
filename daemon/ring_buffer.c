// SPDX-License-Identifier: MIT
#include "ring_buffer.h"
#include "iio_reader.h"

#include <stdlib.h>
#include <string.h>

int ring_buf_init(struct ring_buf *rb, size_t capacity)
{
	rb->samples = calloc(capacity, sizeof(*rb->samples));
	if (!rb->samples)
		return -1;

	rb->capacity = capacity;
	rb->count = 0;
	rb->head = 0;
	return 0;
}

void ring_buf_free(struct ring_buf *rb)
{
	free(rb->samples);
	rb->samples = NULL;
	rb->capacity = 0;
	rb->count = 0;
	rb->head = 0;
}

void ring_buf_push(struct ring_buf *rb, const struct reading *sample)
{
	rb->samples[rb->head] = *sample;
	rb->head = (rb->head + 1) % rb->capacity;
	if (rb->count < rb->capacity)
		rb->count++;
}

const struct reading *ring_buf_get(const struct ring_buf *rb, size_t index)
{
	if (index >= rb->count)
		return NULL;

	size_t pos = (rb->head - rb->count + index + rb->capacity) % rb->capacity;
	return &rb->samples[pos];
}
