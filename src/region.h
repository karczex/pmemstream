// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021-2022, Intel Corporation */

/* Internal Header */

#ifndef LIBPMEMSTREAM_REGION_H
#define LIBPMEMSTREAM_REGION_H

#include "critnib/critnib.h"
#include "libpmemstream.h"
#include "pmemstream_runtime.h"
#include "span.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* After opening pmemstream, each region_runtime is in one of those 2 states.
 * The only possible state transition is: REGION_RUNTIME_STATE_READ_READY -> REGION_RUNTIME_STATE_WRITE_READY
 */
enum region_runtime_state {
	REGION_RUNTIME_STATE_READ_READY, /* reading from the region is safe */
	REGION_RUNTIME_STATE_WRITE_READY /* reading and writing to the region is safe */
};

/*
 * It contains all runtime data specific to a region.
 * It is always managed by the pmemstream (user can only obtain a non-owning pointer) and can be created
 * in few different ways:
 * - By explicitly calling pmemstream_region_runtime_initialize() for the first time
 * - By calling pmemstream_append (only if region_runtime does not exist yet)
 * - By advancing an entry iterator past last entry in a region (only if region_runtime does not exist yet)
 */
struct pmemstream_region_runtime {
	/* State of the region_runtime. */
	enum region_runtime_state state;

	/*
	 * Runtime, needed to perform operations on persistent region.
	 */
	struct pmemstream_runtime *data;

	/*
	 * Points to underlying region.
	 */
	struct pmemstream_region region;

	/*
	 * Offset at which new entries will be appended.
	 */
	uint64_t append_offset;

	/*
	 * All entries which start at offset < committed_offset can be treated as committed and safely read
	 * from multiple threads.
	 *
	 * XXX: committed offset is not reliable in case of concurrent appends to the same region. The reason is
	 * that it is updated in pmemstream_publish/pmemstream_sync, potentially in different order than append_offset.
	 *
	 * To fix this, we might use mechanism similar to timestamp tracking. Alternatively we can batch (transparently
	 * or via a transaction) appends and then, increase committed_offset on batch commit.
	 *
	 * XXX: add test for this: async_append entries of different sizes and call future_poll in different order.
	 */
	uint64_t committed_offset;

	/* Protects region initialization step. */
	pthread_mutex_t region_lock;
};

/**
 * Functions for manipulating regions and region_runtime.
 */

struct region_runtimes_map;

struct region_runtimes_map *region_runtimes_map_new(struct pmemstream_runtime *data);
void region_runtimes_map_destroy(struct region_runtimes_map *map);

/* Gets (or creates if missing) pointer to region_runtime associated with specified region. */
int region_runtimes_map_get_or_create(struct region_runtimes_map *map, struct pmemstream_region region,
				      struct pmemstream_region_runtime **container_handle);
void region_runtimes_map_remove(struct region_runtimes_map *map, struct pmemstream_region region);

/* Precondition: region_runtime_iterate_and_initialize_for_write_locked must have been called. */
uint64_t region_runtime_get_append_offset_acquire(const struct pmemstream_region_runtime *region_runtime);

/* Precondition: region_runtime_iterate_and_initialize_for_write_locked must have been called. */
void region_runtime_increase_append_offset(struct pmemstream_region_runtime *region_runtime, uint64_t diff);
/* Precondition: region_runtime_iterate_and_initialize_for_write_locked must have been called. */
void region_runtime_increase_committed_offset(struct pmemstream_region_runtime *region_runtime, uint64_t diff);

/*
 * Performs region recovery. This function iterates over entire region to find last entry and set append/committed
 * offset appropriately. * After this call, it's safe to write to the region. */
int region_runtime_iterate_and_initialize_for_write_locked(struct pmemstream *stream, struct pmemstream_region region,
							   struct pmemstream_region_runtime *region_runtime);

bool check_entry_consistency(const struct pmemstream_entry_iterator *iterator);

bool check_entry_and_maybe_recover_region(struct pmemstream_entry_iterator *iterator);

uint64_t first_entry_offset(struct pmemstream_region region);

uint64_t region_runtime_get_committed_offset_acquire(const struct pmemstream_region_runtime *region_runtime);

#ifdef __cplusplus
} /* end extern "C" */
#endif
#endif /* LIBPMEMSTREAM_REGION_H */
