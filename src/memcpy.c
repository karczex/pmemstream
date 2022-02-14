// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2022, Intel Corporation */

#include "memcpy.h"
#include "common/util.h"

#include <assert.h>
#include <string.h>

void *pmemstream_memcpy(struct pmemstream *stream, void *destination, const void *source, size_t count)
{
	uint8_t *dest = (uint8_t *)destination;
	uint8_t *src = (uint8_t *)source;

	const size_t dest_missalignment = ALIGN_UP((size_t)dest, CACHELINE_SIZE) - (size_t)dest;

	/* Align destination with cache line */
	if (dest_missalignment > 0) {
		size_t to_align = (dest_missalignment > count) ? count : dest_missalignment;
		stream->memcpy(dest, src, to_align, PMEM2_F_MEM_NONTEMPORAL | PMEM2_F_MEM_NODRAIN);
		if (count <= dest_missalignment) {
			return destination;
		}
		dest += to_align;
		src += to_align;
		count -= to_align;
	}

	assert((size_t)dest % CACHELINE_SIZE == 0);

	size_t not_aligned_tail_size = count % CACHELINE_SIZE;
	size_t alligned_size = count - not_aligned_tail_size;

	assert(alligned_size % CACHELINE_SIZE == 0);

	/* Copy all the data which may be alligned to the cache line */
	if (alligned_size != 0) {
		stream->memcpy(dest, src, alligned_size, PMEM2_F_MEM_NONTEMPORAL | PMEM2_F_MEM_NODRAIN);
		dest += alligned_size;
		src += alligned_size;
	}

	/* Copy rest of the data */

	if (not_aligned_tail_size != 0) {
		stream->memcpy(dest, src, not_aligned_tail_size, PMEM2_F_MEM_TEMPORAL | PMEM2_F_MEM_NODRAIN);
	}
	return destination;
}
