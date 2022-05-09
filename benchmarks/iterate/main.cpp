// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2022, Intel Corporation */

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <forward_list>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "measure.hpp"
/* XXX: Change this header when make_pmemstream moved to public API */
#include "stream_helpers.hpp"

#include <libpmemlog.h>

class config {
 private:
	static constexpr option long_options[] = {{"path", required_argument, NULL, 'p'},
						  {"size", required_argument, NULL, 'x'},
						  {"block_size", required_argument, NULL, 'b'},
						  {"region_size", required_argument, NULL, 'r'},
						  {"iterations", required_argument, NULL, 'i'},
						  {"number_of_regions", required_argument, NULL, 'd'},
						  {"help", no_argument, NULL, 'h'},
						  {NULL, 0, NULL, 0}};

	static std::string app_name;

	void set_size(size_t size_arg)
	{
		if (size_arg < size) {
			throw std::invalid_argument(std::string("Invalid size, should be >=") + std::to_string(size));
		}
		size = size_arg;
	}

 public:
	std::string engine = "pmemstream_regions_iteration";
	std::string path;
	size_t size = std::max(PMEMLOG_MIN_POOL, TEST_DEFAULT_STREAM_SIZE * 10);
	size_t block_size = TEST_DEFAULT_BLOCK_SIZE;
	size_t region_size = TEST_DEFAULT_REGION_SIZE * 10;
	size_t number_of_regions = 1;
	size_t iterations = 10;

	int parse_arguments(int argc, char *argv[])
	{
		app_name = std::string(argv[0]);
		int ch;
		while ((ch = getopt_long(argc, argv, "p:x:b:r:i:t:h", long_options, NULL)) != -1) {
			switch (ch) {
				case 'p':
					path = std::string(optarg);
					break;
				case 'x':
					set_size(std::stoull(optarg));
					break;
				case 'b':
					block_size = std::stoull(optarg);
					break;
				case 'r':
					region_size = std::stoull(optarg);
					break;
				case 'i':
					iterations = std::stoull(optarg);
					break;
				case 'd':
					number_of_regions = std::stoull(optarg);
					break;
				case 'h':
					return -1;
				default:
					throw std::invalid_argument("Invalid argument");
			}
		}
		if (path.empty()) {
			throw std::invalid_argument("Please provide path");
		}
		return 0;
	}

	static void print_usage()
	{
		std::vector<std::string> new_line = {"", ""};
		std::vector<std::vector<std::string>> options = {
			{"Usage: " + app_name + " [OPTION]...\n" + "Log-like structure benchmark for append.", ""},
			new_line,
			{"--path [path]", "path to file"},
			{"--size [size]", "log size"},
			{"--iterations [iterations]", "number of iterations. "},
			new_line,
			{"pmemstream related options:", ""},
			{"--block_size [size]", "block size"},
			{"--region_size [size]", "region size"},
			{"--number_of_regions [count]", "Number of regions to iterate over"},
			new_line,
			{"More iterations gives more robust statistical data, but takes more time", ""},
			{"--help", "display this message"}};
		for (auto &option : options) {
			std::cout << std::setw(25) << std::left << option[0] << " " << option[1] << std::endl;
		}
	}
};

std::string config::app_name;
constexpr option config::long_options[];

std::ostream &operator<<(std::ostream &out, config const &cfg)
{
	out << "Log-like structure Benchmark, path: " << cfg.path << ", ";
	out << "size: " << cfg.size << ", ";
	out << "block_size: " << cfg.block_size << ", ";
	out << "region_size: " << cfg.region_size << ", ";
	out << "number_of_regions: " << cfg.number_of_regions << ", ";
	out << "Number of iterations: " << cfg.iterations << std::endl;
	return out;
}

class pmemstream_region_iteration : public benchmark::workload_base {
 public:
	pmemstream_region_iteration(config &cfg) : cfg(cfg)
	{
		stream = make_pmemstream(cfg.path.c_str(), cfg.block_size, cfg.size);
	}

	virtual void initialize() override
	{
		struct pmemstream_region region;
		for (size_t i = 0; i < cfg.number_of_regions; i++) {
			if (pmemstream_region_allocate(stream.get(), cfg.region_size, &region)) {
				throw std::runtime_error("Error during region allocate!");
			}
		}
		pmemstream_region_iterator_new(&it, stream.get());
	}

	void perform() override
	{
		struct pmemstream_region region;
		while (pmemstream_region_iterator_next(it, &region) == 0) {
		}
	}

	void clean() override
	{

		pmemstream_region_iterator_delete(&it);

		pmemstream_region_iterator *cleanup_iterator;
		pmemstream_region_iterator_new(&cleanup_iterator, stream.get());

		struct pmemstream_region region;
		while (pmemstream_region_iterator_next(cleanup_iterator, &region) == 0) {
			if (pmemstream_region_free(stream.get(), region) != 0) {
				throw std::runtime_error("Error during region free!");
			}
		}
		pmemstream_region_iterator_delete(&cleanup_iterator);
	}

 private:
	config cfg;
	std::unique_ptr<struct pmemstream, std::function<void(struct pmemstream *)>> stream;
	struct pmemstream_region_iterator *it;
};
int main(int argc, char *argv[])
{
	// auto config = parse_arguments(argc, argv);
	config cfg;
	try {
		if (cfg.parse_arguments(argc, argv) != 0) {
			config::print_usage();
			exit(0);
		}
	} catch (std::invalid_argument const &e) {
		std::cerr << e.what() << std::endl;
		exit(1);
	}
	std::cout << cfg << std::endl;

	std::unique_ptr<benchmark::workload_base> workload;

	workload = std::make_unique<pmemstream_region_iteration>(cfg);

	std::vector<std::chrono::nanoseconds::rep> results;
	try {
		results = benchmark::measure<std::chrono::nanoseconds>(cfg.iterations, workload.get(), 1);
	} catch (std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
		return -2;
	}

	auto mean = benchmark::mean(results) / cfg.number_of_regions;
	auto max = static_cast<size_t>(benchmark::max(results)) / cfg.number_of_regions;
	auto min = static_cast<size_t>(benchmark::min(results)) / cfg.number_of_regions;
	auto std_dev = benchmark::std_dev(results) / cfg.number_of_regions;

	std::cout << cfg.engine << " measurement:" << std::endl;
	std::cout << "\tmean[ns]: " << mean << std::endl;
	std::cout << "\tmax[ns]: " << max << std::endl;
	std::cout << "\tmin[ns]: " << min << std::endl;
	std::cout << "\tstandard deviation[ns]: " << std_dev << std::endl;
}
