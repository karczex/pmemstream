// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2022, Intel Corporation */

#ifndef LIBPMEMSTREAM_HPP
#define LIBPMEMSTREAM_HPP

namespace pmem
{

struct stream {
	stream(const std::string &file, size_t block_size, size_t size, bool truncate = true)
	    : c_stream(make_pmemstream(file, block_size, size, truncate))
	{
	}

	stream(const stream &) = delete;
	stream(stream &&) = default;
	stream &operator=(const stream &) = delete;
	stream &operator=(stream &&) = default;

	void close()
	{
		c_stream.reset();
	}

	std::tuple<int, struct pmemstream_region_runtime *> region_runtime_initialize(struct pmemstream_region region)
	{
		struct pmemstream_region_runtime *runtime = nullptr;
		int ret = pmemstream_region_runtime_initialize(c_stream.get(), region, &runtime);
		return {ret, runtime};
	}

	std::tuple<int, struct pmemstream_entry> append(struct pmemstream_region region, const std::string_view &data,
							pmemstream_region_runtime *region_runtime = nullptr)
	{
		pmemstream_entry new_entry = {0};
		auto ret =
			pmemstream_append(c_stream.get(), region, region_runtime, data.data(), data.size(), &new_entry);
		return {ret, new_entry};
	}

	std::tuple<int, struct pmemstream_entry, void *> reserve(struct pmemstream_region region, size_t size,
								 pmemstream_region_runtime *region_runtime = nullptr)
	{
		pmemstream_entry reserved_entry = {0};
		void *reserved_data = nullptr;
		int ret = pmemstream_reserve(c_stream.get(), region, region_runtime, size, &reserved_entry,
					     &reserved_data);
		return {ret, reserved_entry, reserved_data};
	}

	int publish(struct pmemstream_region region, const void *data, size_t size,
		    struct pmemstream_entry reserved_entry, pmemstream_region_runtime *region_runtime = nullptr)
	{
		return pmemstream_publish(c_stream.get(), region, region_runtime, data, size, reserved_entry);
	}

	std::tuple<int, struct pmemstream_region> region_allocate(size_t size)
	{
		pmemstream_region region = {0};
		int ret = pmemstream_region_allocate(c_stream.get(), size, &region);
		return {ret, region};
	}

	size_t region_size(pmemstream_region region)
	{
		return pmemstream_region_size(c_stream.get(), region);
	}

	auto entry_iterator(pmemstream_region region)
	{
		struct pmemstream_entry_iterator *eiter;
		int ret = pmemstream_entry_iterator_new(&eiter, c_stream.get(), region);
		if (ret != 0) {
			throw std::runtime_error("pmemstream_entry_iterator_new failed");
		}

		auto deleter = [](pmemstream_entry_iterator *iter) { pmemstream_entry_iterator_delete(&iter); };
		return std::unique_ptr<struct pmemstream_entry_iterator, decltype(deleter)>(eiter, deleter);
	}

	auto region_iterator()
	{
		struct pmemstream_region_iterator *riter;
		int ret = pmemstream_region_iterator_new(&riter, c_stream.get());
		if (ret != 0) {
			throw std::runtime_error("pmemstream_region_iterator_new failed");
		}

		auto deleter = [](pmemstream_region_iterator *iter) { pmemstream_region_iterator_delete(&iter); };
		return std::unique_ptr<struct pmemstream_region_iterator, decltype(deleter)>(riter, deleter);
	}

	int region_free(pmemstream_region region)
	{
		return pmemstream_region_free(c_stream.get(), region);
	}

	std::string_view get_entry(struct pmemstream_entry entry)
	{
		auto ptr = reinterpret_cast<const char *>(pmemstream_entry_data(c_stream.get(), entry));
		return std::string_view(ptr, pmemstream_entry_length(c_stream.get(), entry));
	}

 private:
	static std::unique_ptr<struct pmemstream, std::function<void(struct pmemstream *)>>
	make_pmemstream(const std::string &file, size_t block_size, size_t size, bool truncate = true);

	std::unique_ptr<struct pmemstream, std::function<void(struct pmemstream *)>> c_stream;

};

std::unique_ptr<struct pmemstream, std::function<void(struct pmemstream *)>>
stream::make_pmemstream(const std::string &file, size_t block_size, size_t size, bool truncate)
{
	struct pmem2_map *map = map_open(file.c_str(), size, truncate);
	if (map == NULL) {
		throw std::runtime_error(pmem2_errormsg());
	}

	auto map_delete = [](struct pmem2_map *map) { pmem2_map_delete(&map); };
	auto map_sptr = std::shared_ptr<struct pmem2_map>(map, map_delete);

	struct pmemstream *stream;
	int ret = pmemstream_from_map(&stream, block_size, map);
	if (ret == -1) {
		throw std::runtime_error("pmemstream_from_map failed");
	}

	auto stream_delete = [map_sptr](struct pmemstream *stream) { pmemstream_delete(&stream); };
	return std::unique_ptr<struct pmemstream, std::function<void(struct pmemstream *)>>(stream, stream_delete);
}

} // namespace pmem

#endif /* LIBPMEMSTREAM_HPP */


