// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * pmemstream_memcpy_pbt.cpp -- pmemstream_memcpy property based tests
 */

#include "common/util.h"
#include "libpmemstream_internal.h"
#include "unittest.hpp"

#include <rapidcheck.h>

#include <algorithm>
#include <vector>
#include <iostream>

static constexpr size_t block_size = 4096;
static constexpr size_t stream_size = 1024 * 1024;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " file" << std::endl;
		return -1;
	}

	auto path = std::string(argv[1]);

	return run_test([&] {
		return_check ret;

		ret += rc::check(
			"verify if all data is copied in correct order", [&](const std::vector<uint64_t> &data) {
				auto stream = make_pmemstream(path, block_size, stream_size);
				auto data_size = data.size() * sizeof(uint64_t);
				std::unique_ptr<uint64_t[]> output(new uint64_t[data.size()]);

				int ret_val = pmemstream_memcpy(stream->memcpy, output.get(), data.data(), data_size);
				RC_ASSERT(ret_val == 0);
				auto res = std::equal(data.begin(), data.end(), output.get());

				RC_ASSERT(res);
			});
	});
}
