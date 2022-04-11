// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021-2022, Intel Corporation */

#include "common/util.h"
#include "span.h"
#include "stream_helpers.h"
#include "unittest.h"

/**
 * region_iterator - unit test for pmemstream_region_iterator_new,
 *					pmemstrem_region_iterator_next, pmemstream_region_iterator_delete
 */

void valid_input_test(char *path)
{
	pmemstream_test_env env = pmemstream_test_make_default(path);

	struct pmemstream_region_iterator *riter;

	struct pmemstream_region region;
	int ret = pmemstream_region_allocate(env.stream, TEST_DEFAULT_REGION_SIZE, &region);
	UT_ASSERTeq(ret, 0);

	ret = pmemstream_region_iterator_new(&riter, env.stream);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTne(riter, NULL);

	ret = pmemstream_region_iterator_next(riter, &region);
	UT_ASSERTeq(ret, 0);

	ret = pmemstream_region_iterator_next(riter, &region);
	UT_ASSERTeq(ret, -1);

	pmemstream_region_iterator_delete(&riter);
	UT_ASSERTeq(riter, NULL);

	pmemstream_region_free(env.stream, region);
	pmemstream_test_teardown(env);
}

void remove_region_while_holding_iterator_test(char *path)
{
	struct pmemstream_test_env env;
	env.map = map_open(path, TEST_DEFAULT_STREAM_SIZE * 10, true);
	int ret = pmemstream_from_map(&env.stream, TEST_DEFAULT_BLOCK_SIZE, env.map);
	UT_ASSERTeq(ret, 0);

	struct pmemstream_region_iterator *riter;

	struct pmemstream_region regions[3];
	for (size_t i = 0; i < 3; i++) {
		int ret = pmemstream_region_allocate(env.stream, TEST_DEFAULT_REGION_SIZE, &regions[i]);
		UT_ASSERTeq(ret, 0);
	}
	ret = pmemstream_region_iterator_new(&riter, env.stream);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTne(riter, NULL);

	/* Go to 2nd region */
	struct pmemstream_region region;
	ret = pmemstream_region_iterator_next(riter, &region);
	UT_ASSERTeq(ret, 0);

	/* Free all regions */
	for (size_t i = 0; i < 3; i++) {
		int ret = pmemstream_region_free(env.stream, regions[i]);
		UT_ASSERTeq(ret, 0);
	}

	struct pmemstream_region region_to_preserve;
	struct pmemstream_region region_to_remove;

	ret = pmemstream_region_allocate(env.stream, TEST_DEFAULT_REGION_SIZE, &region_to_preserve);
	UT_ASSERTeq(ret, 0);

	ret = pmemstream_region_allocate(env.stream, TEST_DEFAULT_REGION_SIZE, &region_to_remove);
	UT_ASSERTeq(ret, 0);

	ret = pmemstream_region_free(env.stream, region_to_remove);
	UT_ASSERTeq(ret, 0);

	/* let's iterate over free list */
	struct pmemstream_region region_from_free_list;
	ret = pmemstream_region_iterator_next(riter, &region_from_free_list);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(region_from_free_list.offset, regions[1].offset);

	ret = pmemstream_region_iterator_next(riter, &region_from_free_list);
	UT_ASSERTeq(region_from_free_list.offset, region_to_remove.offset);

	pmemstream_region_iterator_delete(&riter);
	UT_ASSERTeq(riter, NULL);

	pmemstream_test_teardown(env);
}

void null_iterator_test(char *path)
{
	pmemstream_test_env env = pmemstream_test_make_default(path);

	int ret;
	struct pmemstream_region region;
	ret = pmemstream_region_allocate(env.stream, TEST_DEFAULT_REGION_SIZE, &region);
	UT_ASSERTeq(ret, 0);

	ret = pmemstream_region_iterator_new(NULL, env.stream);
	UT_ASSERTeq(ret, -1);

	ret = pmemstream_region_iterator_next(NULL, &region);
	UT_ASSERTeq(ret, -1);

	pmemstream_region_free(env.stream, region);
	pmemstream_test_teardown(env);
}

void invalid_region_test(char *path)
{
	pmemstream_test_env env = pmemstream_test_make_default(path);

	const uint64_t invalid_offset = ALIGN_DOWN(UINT64_MAX, sizeof(span_bytes));
	struct pmemstream_region_iterator *riter = NULL;
	struct pmemstream_region invalid_region = {.offset = invalid_offset};

	int ret = pmemstream_region_iterator_new(&riter, env.stream);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTne(riter, NULL);

	ret = pmemstream_region_iterator_next(riter, &invalid_region);
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(invalid_region.offset, invalid_offset);

	pmemstream_region_iterator_delete(&riter);
	pmemstream_test_teardown(env);
}

void null_stream_test()
{
	struct pmemstream_region_iterator *riter = NULL;
	int ret;

	ret = pmemstream_region_iterator_new(&riter, NULL);
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(riter, NULL);

	pmemstream_region_iterator_delete(&riter);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	START();

	char *path = argv[1];
	remove_region_while_holding_iterator_test(path);
	valid_input_test(path);
	null_iterator_test(path);
	invalid_region_test(path);
	null_stream_test();

	return 0;
}
