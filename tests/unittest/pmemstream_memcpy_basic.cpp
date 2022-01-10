// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2022, Intel Corporation */

/*
 * pmemstream_memcpy_basic.cpp -- pmemstream_memcpy basic tests, which may be
 * run if rapidcheck is not available
 */

#include "common/util.h"
#include "libpmemstream_internal.h"
#include "unittest.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

static constexpr size_t stream_size = 1024 * 1024;
static constexpr size_t block_size = 4096;

struct aligned_struct {
	size_t x;
	size_t y;
	size_t other_data[10];
	size_t padding[4];
	size_t aligned_data[8];
};

/* Test basic functionality of memcpy with cache line aligned data.
 * pmemstream is needed only to get memcpy implementation, as the test copy data to the dram structure */
static void test_basic(std::string path)
{
	std::cout << "starting basic test" << std::endl;
	auto stream = make_pmemstream(path, block_size, stream_size);

	aligned_struct to_fill;
	size_t a = 2;
	size_t b = 1;
	size_t c[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	size_t d[] = {21, 22, 23, 24};
	size_t e[] = {31, 32, 33, 34, 35, 36, 37, 38};
	auto ret = pmemstream_memcpy(stream->memcpy, &to_fill, &a, sizeof(a), &b, sizeof(b), c, sizeof(c), d, sizeof(d),
				     e, sizeof(e));
	std::cout << "data copyied" << std::endl;

	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(to_fill.x, a);
	UT_ASSERTeq(to_fill.y, b);
	for (size_t i = 0; i < sizeof(c) / sizeof(c[0]); i++) {
		UT_ASSERTeq(to_fill.other_data[i], c[i]);
	}
	for (size_t i = 0; i < sizeof(d) / sizeof(d[0]); i++) {
		UT_ASSERTeq(to_fill.padding[i], d[i]);
	}
	for (size_t i = 0; i < sizeof(e) / sizeof(e[0]); i++) {
		UT_ASSERTeq(to_fill.aligned_data[i], e[i]);
	}
}

/* Test memcpy with data not aligned to the cache line */
static void test_not_aligned_array(std::string path)
{

	std::cout << "starting not aligned array test" << std::endl;
	auto stream = make_pmemstream(path, block_size, stream_size);

	std::vector<size_t> not_aligned_data = {0x01, 0x02};
	std::vector<size_t> buf(not_aligned_data.size());

	pmemstream_memcpy(stream->memcpy, buf.data(), not_aligned_data.data(),
			  not_aligned_data.size() * sizeof(not_aligned_data[0]));

	std::cout << "data copyied" << std::endl;
	UT_ASSERT(std::equal(not_aligned_data.begin(), not_aligned_data.end(), buf.begin()));
}

/* Test memcpy with  zero sized element as argument */
static void test_zero_sized_parameter(std::string path)
{

	std::cout << "starting not zero parameter test" << std::endl;
	auto stream = make_pmemstream(path, block_size, stream_size);

	size_t to_fill[2];

	size_t a = 1;
	size_t b = 2;

	auto ret = pmemstream_memcpy(stream->memcpy, &to_fill, &a, sizeof(a), nullptr, 0, &b, sizeof(b));

	std::cout << "data copyied" << std::endl;
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(a, to_fill[0]);
	UT_ASSERTeq(b, to_fill[1]);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " file" << std::endl;
		return -1;
	}

	auto path = std::string(argv[1]);

	return run_test([&] {
		test_basic(path);
		test_not_aligned_array(path);
		test_zero_sized_parameter(path);
	});
}
