// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021-2022, Intel Corporation */

/* Common, internal utils */

#ifndef LIBPMEMSTREAM_UTIL_H
#define LIBPMEMSTREAM_UTIL_H

#include <stddef.h>
#include <stdint.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define ALIGN_UP(size, align) (((size) + (align)-1) & ~((align)-1))
#define ALIGN_DOWN(size, align) ((size) & ~((align)-1))

#define MEMBER_SIZE(type, member) sizeof(((struct type *)NULL)->member)

#if defined(__x86_64) || defined(_M_X64) || defined(__aarch64__) || defined(__riscv)
#define CACHELINE_SIZE 64ULL
#elif defined(__PPC64__)
#define CACHELINE_SIZE 128ULL
#else
#error unable to recognize architecture at compile time
#endif

static inline unsigned char util_popcount64(uint64_t value)
{
	return (unsigned char)__builtin_popcountll(value);
}

static inline size_t util_popcount_memory(const uint8_t *data, size_t size)
{
	size_t count = 0;
	size_t i = 0;

	for (; i < ALIGN_DOWN(size, sizeof(uint64_t)); i += sizeof(uint64_t)) {
		count += util_popcount64(*(const uint64_t *)(data + i));
	}
	for (; i < size; i++) {
		count += util_popcount64(data[i]);
	}

	return count;
}

#endif /* LIBPMEMSTREAM_UTIL_H */
