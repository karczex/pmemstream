// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021-2022, Intel Corporation */

#ifndef LIBPMEMSTREAM_STREAM_HELPERS_HPP
#define LIBPMEMSTREAM_STREAM_HELPERS_HPP

#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "libpmemstream.hpp"
#include "unittest.hpp"

/* Implements additional functions, useful for testing. */
struct pmemstream_helpers_type {
	pmemstream_helpers_type(pmem::stream &stream, bool call_region_runtime_initialize)
	    : stream(stream), call_region_runtime_initialize(call_region_runtime_initialize)
	{
	}

	pmemstream_helpers_type(const pmemstream_helpers_type &) = delete;
	pmemstream_helpers_type(pmemstream_helpers_type &&) = delete;
	pmemstream_helpers_type &operator=(const pmemstream_helpers_type &) = delete;
	pmemstream_helpers_type &operator=(pmemstream_helpers_type &&) = delete;

	void append(struct pmemstream_region region, const std::vector<std::string> &data)
	{
		for (const auto &e : data) {
			auto [ret, entry] = stream.append(region, e, region_runtime[region.offset]);
			UT_ASSERTeq(ret, 0);
		}
	}

	struct pmemstream_region initialize_single_region(size_t region_size, const std::vector<std::string> &data)
	{
		auto [ret, region] = stream.region_allocate(region_size);
		UT_ASSERTeq(ret, 0);
		/* region_size is aligned up to block_size, on allocation, so it may be bigger than expected */
		UT_ASSERT(stream.region_size(region) >= region_size);

		if (call_region_runtime_initialize) {
			auto [ret, runtime] = stream.region_runtime_initialize(region);
			region_runtime[region.offset] = runtime;
		}

		append(region, data);

		return region;
	}

	/* Reserve space, write data, and publish (persist) them, within the given region.
	 * Do this for all data in the vector. */
	void reserve_and_publish(struct pmemstream_region region, const std::vector<std::string> &data)
	{
		for (const auto &d : data) {
			/* reserve space for given data */
			auto [ret, reserved_entry, reserved_data] =
				stream.reserve(region, d.size(), region_runtime[region.offset]);
			UT_ASSERTeq(ret, 0);

			/* write into the reserved space and publish (persist) it */
			memcpy(reserved_data, d.data(), d.size());

			auto ret_p = stream.publish(region, reinterpret_cast<const void *>(d.data()), d.size(),
						    reserved_entry, region_runtime[region.offset]);
			UT_ASSERTeq(ret_p, 0);
		}
	}

	struct pmemstream_region get_first_region()
	{
		auto riter = stream.region_iterator();

		struct pmemstream_region region;
		auto ret = pmemstream_region_iterator_next(riter.get(), &region);
		UT_ASSERTne(ret, -1);

		return region;
	}

	struct pmemstream_entry get_last_entry(pmemstream_region region)
	{
		auto eiter = stream.entry_iterator(region);

		struct pmemstream_entry last_entry = {0};
		struct pmemstream_entry tmp_entry;
		while (pmemstream_entry_iterator_next(eiter.get(), nullptr, &tmp_entry) == 0) {
			last_entry = tmp_entry;
		}

		if (last_entry.offset == 0)
			throw std::runtime_error("No elements in this region.");

		UT_ASSERTeq(last_entry.offset, tmp_entry.offset);

		return last_entry;
	}

	std::vector<std::string> get_elements_in_region(struct pmemstream_region region)
	{
		std::vector<std::string> result;

		auto eiter = stream.entry_iterator(region);
		struct pmemstream_entry entry;
		struct pmemstream_region r;
		while (pmemstream_entry_iterator_next(eiter.get(), &r, &entry) == 0) {
			UT_ASSERTeq(r.offset, region.offset);
			result.emplace_back(stream.get_entry(entry));
		}

		return result;
	}

	/* XXX: extend to allow more than one extra_data vector */
	void verify(pmemstream_region region, const std::vector<std::string> &data,
		    const std::vector<std::string> &extra_data)
	{
		/* Verify if stream now holds data + extra_data */
		auto all_elements = get_elements_in_region(region);
		auto extra_data_start = all_elements.begin() + static_cast<int>(data.size());

		UT_ASSERTeq(all_elements.size(), data.size() + extra_data.size());
		UT_ASSERT(std::equal(all_elements.begin(), extra_data_start, data.begin()));
		UT_ASSERT(std::equal(extra_data_start, all_elements.end(), extra_data.begin()));
	}

	pmem::stream &stream;
	std::map<uint64_t, pmemstream_region_runtime *> region_runtime;
	bool call_region_runtime_initialize = false;
};

struct pmemstream_test_base {
	pmemstream_test_base(const std::string &file, size_t block_size, size_t size, bool truncate = true,
			     bool call_initialize_region_runtime = false,
			     bool call_initialize_region_runtime_after_reopen = false)
	    : sut(file, block_size, size, truncate),
	      file(file),
	      block_size(block_size),
	      size(size),
	      helpers(sut, call_initialize_region_runtime),
	      call_initialize_region_runtime(call_initialize_region_runtime),
	      call_initialize_region_runtime_after_reopen(call_initialize_region_runtime_after_reopen)
	{
	}

	pmemstream_test_base(const pmemstream_test_base &) = delete;
	pmemstream_test_base(pmemstream_test_base &&rhs)
	    : sut(std::move(rhs.sut)),
	      file(rhs.file),
	      block_size(rhs.block_size),
	      size(rhs.size),
	      helpers(sut, rhs.call_initialize_region_runtime),
	      call_initialize_region_runtime(rhs.call_initialize_region_runtime),
	      call_initialize_region_runtime_after_reopen(rhs.call_initialize_region_runtime_after_reopen)
	{
	}

	/* This function closes and reopens the stream. All pointers to stream data, iterators, etc. are invalidated. */
	void reopen()
	{
		sut.close();

		sut = pmem::stream(file, block_size, size, false);

		if (call_initialize_region_runtime_after_reopen) {
			for (auto &r : helpers.region_runtime) {
				auto [ret, runtime] = sut.region_runtime_initialize(pmemstream_region{r.first});
				UT_ASSERTeq(ret, 0);
				r.second = runtime;
			}
		} else {
			/* Invalidate. */
			helpers.region_runtime.clear();
		}
	}

	/* Stream under test. */
	pmem::stream sut;

	/* Arguments used to create the stream. */
	std::string file;
	size_t block_size;
	size_t size;

	pmemstream_helpers_type helpers;

	bool call_initialize_region_runtime = false;
	bool call_initialize_region_runtime_after_reopen = false;
};

auto make_default_test_stream()
{
	return pmemstream_test_base(get_test_config().filename, get_test_config().block_size,
				    get_test_config().stream_size, true);
}

struct pmemstream_with_single_init_region : public pmemstream_test_base {
	pmemstream_with_single_init_region(pmemstream_test_base &&base, const std::vector<std::string> &data)
	    : pmemstream_test_base(std::move(base))
	{
		helpers.initialize_single_region(TEST_DEFAULT_REGION_SIZE, data);
	}
};

struct pmemstream_with_single_empty_region : public pmemstream_test_base {
	pmemstream_with_single_empty_region(pmemstream_test_base &&base) : pmemstream_test_base(std::move(base))
	{
		helpers.initialize_single_region(TEST_DEFAULT_REGION_SIZE, {});
	}
};

struct pmemstream_empty : public pmemstream_test_base {
	pmemstream_empty(pmemstream_test_base &&base) : pmemstream_test_base(std::move(base))
	{
	}
};

#endif /* LIBPMEMSTREAM_STREAM_HELPERS_HPP */
