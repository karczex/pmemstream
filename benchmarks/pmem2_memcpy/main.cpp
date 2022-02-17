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
#include <regex>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <libpmem2.h>

#include "measure.hpp"
/* XXX: Change this header when make_pmemstream moved to public API */
#include "common/util.h"
#include "unittest.hpp"

class config {
 private:
	static constexpr std::array<std::string_view, 2> engine_names = {"pmemlog", "pmemstream"};
	static constexpr option long_options[] = {{"path", required_argument, NULL, 'p'},
						  {"size", required_argument, NULL, 'x'},
						  {"element_count", required_argument, NULL, 'c'},
						  {"element_size", required_argument, NULL, 's'},
						  {"flags", required_argument, NULL, 'f'},
						  {"iterations", required_argument, NULL, 'i'},
						  {"persist", no_argument, NULL, 'r'},
						  {"align", required_argument, NULL, 'a'},
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

	uint64_t parse_flags(std::string flags)
	{
		std::unordered_map<std::string, uint64_t> tokens = {
			{"PMEM2_F_MEM_NODRAIN", PMEM2_F_MEM_NODRAIN},
			{"PMEM2_F_MEM_NOFLUSH", PMEM2_F_MEM_NOFLUSH},
			{"PMEM2_F_MEM_NONTEMPORAL", PMEM2_F_MEM_NONTEMPORAL},
			{"PMEM2_F_MEM_TEMPORAL", PMEM2_F_MEM_TEMPORAL},
			{"PMEM2_F_MEM_WC", PMEM2_F_MEM_WC},
			{"PMEM2_F_MEM_WB", PMEM2_F_MEM_WB}};
		uint64_t result = 0;
		std::regex rgx("\\w+");

		for (std::sregex_iterator it(flags.begin(), flags.end(), rgx), it_end; it != it_end; ++it) {
			std::string token = (*it)[0];
			auto flag = tokens.find(token);
			if (flag != tokens.end()) {
				result |= flag->second;
			} else {
				throw std::invalid_argument("Invalid flag " + token);
			}
		}
		return result;
	}

 public:
	std::string path;
	size_t element_count = 100;
	size_t element_size = 1024;
	size_t size = element_count * element_size;
	size_t iterations = 10;
	uint64_t flags = 0;
	std::string flags_names = "0";
	bool persist_after_memcpy = false;
	size_t alignment = CACHELINE_SIZE;

	int parse_arguments(int argc, char *argv[])
	{
		app_name = std::string(argv[0]);
		int ch;
		while ((ch = getopt_long(argc, argv, "p:x:c:s:i:f:ra:h", long_options, NULL)) != -1) {
			switch (ch) {
				case 'p':
					path = std::string(optarg);
					break;
				case 'x':
					set_size(std::stoull(optarg));
					break;
				case 'c':
					element_count = std::stoull(optarg);
					break;
				case 's':
					element_size = std::stoull(optarg);
					break;
				case 'i':
					iterations = std::stoull(optarg);
					break;
				case 'f':
					flags_names = std::string(optarg);
					flags = parse_flags(flags_names);
					break;
				case 'r':
					persist_after_memcpy = true;
					break;
				case 'a':
					alignment = std::stoull(optarg);
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
			{"Usage: " + app_name + " [OPTION]...\n" + "pmem2_memcpy benchmark for append only workload",
			 ""},
			new_line,
			{"--path [path]", "path to file"},
			{"--size [size]", "log size"},
			{"--element_count [count]", "number of elements to be inserted"},
			{"--element_size [size]", "number of bytes of each element"},
			{"--memcpy flags [flags]", "flags passed to memcpy"},
			{"--iterations [iterations]", "number of iterations. "},
			new_line,
			{"More iterations gives more robust statistical data, but takes more time", ""},
			{"--persist", "Do persist after memcpy"},
			{"--align", "Align destination to cache line. False by default."},
			{"--help", "display this message"}};
		for (auto &option : options) {
			std::cout << std::setw(25) << std::left << option[0] << " " << option[1] << std::endl;
		}
	}
};
std::string config::app_name;
constexpr option config::long_options[];
constexpr std::array<std::string_view, 2> config::engine_names;

std::ostream &operator<<(std::ostream &out, config const &cfg)
{
	out << "pmem2_memcpy, path: " << cfg.path << ", ";
	out << "size: " << cfg.size << ", ";
	out << "element_count: " << cfg.element_count << ", ";
	out << "element_size: " << cfg.element_size << ", ";
	out << "flags: " << cfg.flags_names << ", ";
	out << "destination_alignment: " << cfg.alignment << ", ";
	out << "persist_after_memcpy " << std::boolalpha << cfg.persist_after_memcpy << ", ";
	out << "Number of iterations: " << cfg.iterations << std::endl;
	return out;
}

class pmem2_workload : public benchmark::workload_base {
 public:
	pmem2_workload(config &cfg) : cfg(cfg)
	{
	}

	virtual void initialize() override
	{
		auto bytes_to_generate = cfg.element_count * cfg.element_size;
		prepare_data(bytes_to_generate);
		map = map_open(cfg.path.c_str(), cfg.size, true);
		if (map == NULL) {
			throw std::runtime_error(pmem2_errormsg());
		}

		pmem2_memcpy = pmem2_get_memcpy_fn(map);
		pmem2_persist = pmem2_get_persist_fn(map);
		destination = reinterpret_cast<uint8_t *>(
			ALIGN_UP(reinterpret_cast<uintptr_t>(pmem2_map_get_address(map)), CACHELINE_SIZE));
	}
	void perform() override
	{
		auto data_chunks = get_data_chunks();
		for (size_t i = 0; i < data.size() * sizeof(uint64_t); i += cfg.element_size) {
			pmem2_memcpy(destination + i, data_chunks + i, cfg.element_size, cfg.flags);
			if (cfg.persist_after_memcpy) {
				pmem2_persist(destination, cfg.element_size);
			}
		}
	}

	void clean() override
	{
		pmem2_map_delete(&map);
	}

 private:
	config cfg;
	pmem2_map *map;
	pmem2_memcpy_fn pmem2_memcpy;
	pmem2_persist_fn pmem2_persist;
	uint8_t *destination;
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

	std::unique_ptr<benchmark::workload_base> workload = std::make_unique<pmem2_workload>(cfg);

	/* XXX: Add initialization phase whith separate measurement */
	std::vector<std::chrono::nanoseconds::rep> results;
	try {
		results = benchmark::measure<std::chrono::nanoseconds>(cfg.iterations, workload.get());
	} catch (std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
		return -2;
	}

	auto mean = benchmark::mean(results) / cfg.element_count;
	auto max = static_cast<size_t>(benchmark::max(results)) / cfg.element_count;
	auto min = static_cast<size_t>(benchmark::min(results)) / cfg.element_count;
	auto std_dev = benchmark::std_dev(results) / cfg.element_count;

	std::cout << " measurement:" << std::endl;
	std::cout << "\tmean[ns]: " << mean << std::endl;
	std::cout << "\tmax[ns]: " << max << std::endl;
	std::cout << "\tmin[ns]: " << min << std::endl;
	std::cout << "\tstandard deviation[ns]: " << std_dev << std::endl;
}
