// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021-2022, Intel Corporation */

#include "examples_helpers.h"
#include "libpmemstream.hpp"

#include <libpmem2.h>
#include <stdio.h>

struct data_entry {
	uint64_t data;
};

/**
 * This example creates a stream from map2 source, prints its content,
 * and appends monotonically increasing values at the end.
 *
 * It creates a file at given path, with size = STREAM_SIZE.
 */
int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s file\n", argv[0]);
		return -1;
	}

	struct pmem2_map *map = example_map_open(argv[1], EXAMPLE_STREAM_SIZE);
	if (map == NULL) {
		pmem2_perror("pmem2_map");
		return -1;
	}

	struct pmem::stream::pmemstream *stream;
	int ret = pmem::stream::pmemstream_from_map(&stream, 4096, map);
	if (ret == -1) {
		fprintf(stderr, "pmemstream_from_map failed\n");
		return ret;
	}

	struct pmem::stream::pmemstream_region_iterator *riter;
	ret = pmem::stream::pmemstream_region_iterator_new(&riter, stream);
	if (ret == -1) {
		fprintf(stderr, "pmemstream_region_iterator_new failed\n");
		return ret;
	}

	struct pmem::stream::pmemstream_region region;

	/* Iterate over all regions. */
	while (pmem::stream::pmemstream_region_iterator_next(riter, &region) == 0) {
		struct pmem::stream::pmemstream_entry entry;
		struct pmem::stream::pmemstream_entry_iterator *eiter;
		ret = pmem::stream::pmemstream_entry_iterator_new(&eiter, stream, region);
		if (ret == -1) {
			fprintf(stderr, "pmemstream_entry_iterator_new failed\n");
			return ret;
		}

		/* Iterate over all elements in a region and save last entry value. */
		uint64_t last_entry_data;
		while (pmem::stream::pmemstream_entry_iterator_next(eiter, NULL, &entry) == 0) {
			struct data_entry *d = static_cast<data_entry*>(pmem::stream::pmemstream_entry_data(stream, entry));
			printf("data entry %lu: %lu in region %lu\n", entry.offset, d->data, region.offset);
			last_entry_data = d->data;
		}
		pmem::stream::pmemstream_entry_iterator_delete(&eiter);

		struct data_entry e;
		e.data = last_entry_data + 1;
		struct pmem::stream::pmemstream_entry new_entry;
		ret = pmem::stream::pmemstream_append(stream, region, NULL, &e, sizeof(e), &new_entry);
		if (ret == -1) {
			fprintf(stderr, "pmemstream_append failed\n");
			return ret;
		}
	}

	pmem::stream::pmemstream_region_iterator_delete(&riter);


	pmem::stream::pmemstream_delete(&stream);
	pmem2_map_delete(&map);

	return 0;
}
