// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021-2022, Intel Corporation */

/* Internal Header */

#ifndef LIBPMEMSTREAM_MEMCPY_H
#define LIBPMEMSTREAM_MEMCPY_H

#include "libpmemstream_internal.h"

#include <libpmem2.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// XXX: Do we want to add it to library API? It may be usefull for reserve-publish scenario
void *pmemstream_memcpy(struct pmemstream *stream, void *destination, const void *source, size_t count);

#ifdef __cplusplus
} /* end extern "C" */
#endif
#endif /* LIBPMEMSTREAM_MEMCPY_H */
