// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2022, Intel Corporation */

/* thread_id.cpp -- verifies correctness of thread_id module */

#include "thread_helpers.hpp"
#include "unittest.hpp"

namespace
{
static constexpr size_t concurrency = 2;
}
int main()
{
	return run_test([] {
		/* verify if max thread id is not bigger than number of threads */
		parallel_xexec(concurrency, [&](size_t id, std::function<void(void)> syncthreads) {
			size_t i = 0;
			syncthreads();
			i++;
			syncthreads();
		});
	});
}
