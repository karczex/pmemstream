// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2022, Intel Corporation */

#include "common/util.h"

#include <assert.h>
#include <string.h>

struct sized_ptr {
	uint8_t *ptr;
	size_t size;
};

struct buffer {
	struct sized_ptr head;
	struct sized_ptr body;
	struct sized_ptr tail;
};

int pmemstream_memcpy_impl(pmem2_memcpy_fn pmem2_memcpy, void *destination, const size_t argc, ...)
{

	struct {
		uint8_t array[CACHELINE_SIZE];
		size_t offset;
	} tmp_buffer = {0};

	uint8_t *dest = (uint8_t *)destination;

	/* Parse variadic arguments */
	va_list argv;

	assert(argc % 2 == 0);

	const size_t parts = argc / 2;
	const size_t last_part = parts - 1;

	va_start(argv, argc);
	for (size_t i = 0; i < parts; i++) {

		struct sized_ptr input = {0};
		input.ptr = va_arg(argv, uint8_t *);
		input.size = va_arg(argv, size_t);

		struct buffer buff = {0};

		size_t tmp_buffer_free_space = CACHELINE_SIZE - tmp_buffer.offset;

		/* Compute sizes of memory chunks to copy */
		if (tmp_buffer.offset == 0) {
			buff.head.size = 0;
		} else if (tmp_buffer_free_space >= input.size) {
			buff.head.size = input.size;
		} else {
			buff.head.size = tmp_buffer_free_space;
		}
		buff.tail.size = (input.size - buff.head.size) % CACHELINE_SIZE;
		buff.body.size = input.size - buff.head.size - buff.tail.size;

		assert(buff.head.size >= 0);
		assert(buff.body.size >= 0);
		assert(buff.tail.size >= 0);

		/* Compute chunks pointers */
		buff.head.ptr = input.ptr;
		buff.body.ptr = buff.head.ptr + buff.head.size;
		buff.tail.ptr = buff.body.ptr + buff.body.size;

		/* Do memcpy */

		/* Copy head to temporary buffer */
		if (buff.head.size > 0) {
			memcpy(tmp_buffer.array + tmp_buffer.offset / sizeof(tmp_buffer.array[0]), buff.head.ptr,
			       buff.head.size);
			tmp_buffer.offset += buff.head.size;
		}

		assert(tmp_buffer.offset <= CACHELINE_SIZE);

		/* Copy temporary buffer to pmem */
		if (tmp_buffer.offset == CACHELINE_SIZE) {
			pmem2_memcpy(dest, tmp_buffer.array, CACHELINE_SIZE,
				     PMEM2_F_MEM_NONTEMPORAL | PMEM2_F_MEM_NODRAIN);
			tmp_buffer.offset = 0;
			dest += CACHELINE_SIZE;
		}

		/* Copy body to pmem */
		if (buff.body.size > 0) {
			unsigned flags = PMEM2_F_MEM_NONTEMPORAL;
			if ((i != last_part) || (buff.tail.size != 0)) {
				flags |= PMEM2_F_MEM_NODRAIN;
			}

			pmem2_memcpy(dest, buff.body.ptr, buff.body.size, flags);
			dest += buff.body.size;
		}

		/* Copy tail to temporary buffer */
		if (buff.tail.size > 0) {
			memcpy(tmp_buffer.array + tmp_buffer.offset / sizeof(tmp_buffer.array[0]), buff.tail.ptr,
			       buff.tail.size);
			tmp_buffer.offset += buff.tail.size;
		}
	}

	assert(tmp_buffer.offset <= CACHELINE_SIZE);

	/* Copy rest of the data from temporary buffer to pmem */
	if (tmp_buffer.offset > 0) {
		pmem2_memcpy(dest, tmp_buffer.array, tmp_buffer.offset, PMEM2_F_MEM_NONTEMPORAL | PMEM2_F_MEM_NODRAIN);
	}

	va_end(argv);

	return 0;
}

