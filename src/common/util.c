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

struct write_combining_buffer {
	uint8_t array[CACHELINE_SIZE];
	size_t offset;
};

static void memcpy_to_tmp_buffer(struct write_combining_buffer *dest, struct sized_ptr src)
{
	memcpy(dest->array + dest->offset, src.ptr, src.size);
	dest->offset += src.size;
}

static void memcpy_directly_to_pmem(pmem2_memcpy_fn pmem2_memcpy, uint8_t **dest, struct sized_ptr src, unsigned flags)
{
	pmem2_memcpy(*dest, src.ptr, src.size, flags);
	*dest += src.size;
}

static void memcpy_tmp_buffer_to_pmem(pmem2_memcpy_fn pmem2_memcpy, uint8_t **dest, struct write_combining_buffer *src,
				      unsigned flags)
{
	pmem2_memcpy(*dest, src->array, src->offset, flags);
	*dest += src->offset;
	src->offset = 0;
}

int pmemstream_memcpy_impl(pmem2_memcpy_fn pmem2_memcpy, void *destination, const size_t argc, ...)
{

	struct write_combining_buffer tmp_buffer = {0};

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

		if (input.size == 0) {
			continue;
		}

		/* Align destination to cache line before copying with write
		 * combining */
		const size_t missalignment = ALIGN_UP((size_t)dest, CACHELINE_SIZE) - (size_t)dest;

		if (missalignment > 0) {
			struct sized_ptr input_part;
			input_part.ptr = input.ptr;
			input_part.size = (missalignment > input.size) ? input.size : missalignment;
			memcpy_directly_to_pmem(pmem2_memcpy, &dest, input_part, 0);
			if (input.size <= missalignment) {
				continue;
			}
			input.ptr += input_part.size;
			input.size -= input_part.size;
		}

		assert((size_t)dest % CACHELINE_SIZE == 0);

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

		/* Compute chunks pointers */
		buff.head.ptr = input.ptr;
		buff.body.ptr = buff.head.ptr + buff.head.size;
		buff.tail.ptr = buff.body.ptr + buff.body.size;

		/* Do memcpy */

		/* Copy head to temporary buffer */
		if (buff.head.size > 0) {
			memcpy_to_tmp_buffer(&tmp_buffer, buff.head);
		}

		assert(tmp_buffer.offset <= CACHELINE_SIZE);

		/* Copy temporary buffer to pmem */
		if (tmp_buffer.offset == CACHELINE_SIZE) {
			memcpy_tmp_buffer_to_pmem(pmem2_memcpy, &dest, &tmp_buffer,
						  PMEM2_F_MEM_NONTEMPORAL | PMEM2_F_MEM_NODRAIN);
		}

		/* Copy body to pmem */
		if (buff.body.size > 0) {
			unsigned flags = PMEM2_F_MEM_NONTEMPORAL;
			if ((i != last_part) || (buff.tail.size != 0)) {
				flags |= PMEM2_F_MEM_NODRAIN;
			}
			memcpy_directly_to_pmem(pmem2_memcpy, &dest, buff.body, flags);
		}

		/* Copy tail to temporary buffer */
		if (buff.tail.size > 0) {
			memcpy_to_tmp_buffer(&tmp_buffer, buff.tail);
		}
	}

	assert(tmp_buffer.offset <= CACHELINE_SIZE);

	/* Copy rest of the data from temporary buffer to pmem */
	if (tmp_buffer.offset > 0) {
		memcpy_tmp_buffer_to_pmem(pmem2_memcpy, &dest, &tmp_buffer,
					  PMEM2_F_MEM_NONTEMPORAL | PMEM2_F_MEM_NODRAIN);
	}

	va_end(argv);

	return 0;
}
