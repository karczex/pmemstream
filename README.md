# pmemstream

[![Basic Tests](https://github.com/pmem/pmemstream/actions/workflows/basic.yml/badge.svg)](https://github.com/pmem/pmemstream/actions/workflows/basic.yml)
[![pmemstream version](https://img.shields.io/github/tag/pmem/pmemstream.svg)](https://github.com/pmem/pmemstream/releases/latest)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/24120/badge.svg)](https://scan.coverity.com/projects/pmem-pmemstream)
[![Coverage Status](https://codecov.io/github/pmem/pmemstream/coverage.svg?branch=master)](https://app.codecov.io/gh/pmem/pmemstream/branch/master)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/pmem/pmemstream.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/pmem/pmemstream/context:cpp)

`pmemstream` implements a persistent memory optimized log data structure and provides stream-like access to data.
It presents a contiguous logical address space, divided into regions, with log entries of arbitrary size.
Purpose of this library is to be a foundation for various, more complex higher-level solutions. Like most libraries in the PMDK family, this one also focuses on delivering a generic, easy-to-use set of functions.

## Project objectives:

 * Low latency writes determined by underlying memory capabilities
 * Low write aplification
 * Low metadata overhead
 * Power fail safety
 * Does not require extensive knowledge of PMEM

`pmemstream` is concepual successor of [pmemlog](https://pmem.io/pmdk/libpmemlog/), however it provides additional features which suits better for multithreaded environments:

* Independent regions - which allows for thread-safe appends.
* Region, and entry iterators
* Asynchronous append operation - via [miniasync library](https://github.com/pmem/miniasync/)

and more features in the future:

* [Timestamps](https://github.com/pmem/pmemstream/issues/78)
* [Transactions](https://github.com/pmem/pmemstream/issues/77)
* [Concurrent appends to single region](https://github.com/pmem/pmemstream/issues/79)
* [Access from multiple processes](https://github.com/pmem/pmemstream/issues/75) in single writer-multiple readers mode

*This is experimental pre-release software and should not be used in production systems.
APIs and file formats may change at any time without preserving backwards compatibility.
All known issues and limitations are logged as GitHub issues.*

## Table of contents
1. [Installation](#installation)
2. [Structure](#structure)
3. [Contact us](#contact-us)

## Installation

[Installation guide](INSTALL.md) provides detailed instructions how to build and install
`pmemstream` from sources, build rpm and deb packages, and more.

## Structure

Contiguous logical address space returned by [pmem2_map_new()](https://pmem.io/pmdk/manpages/linux/master/libpmem2/pmem2_map_new.3/) is divided into dynamically allocated independent regions of size specified by the user. To each such created region, entries of arbitrary sizes may be appended. As onece created region has fixed size it's guaranteed that appends to one region would not override data in any other region.

```
├──  region0: 8128 bytes
│   ├── 0x40  8bytes 01 00 00 00 00 00 00 00
│   ├── 0x58  8bytes 02 00 00 00 00 00 00 00
│   ├── 0x70  8bytes 03 00 00 00 00 00 00 00
│   ├── 0x88  8bytes 04 00 00 00 00 00 00 00
│   ├── 0xA0  8bytes 05 00 00 00 00 00 00 00
│   ├── 0xB8  8bytes 06 00 00 00 00 00 00 00
│   ├── 0xD0  8bytes 07 00 00 00 00 00 00 00
├──  region1: 8128 bytes
│   ├── 0x2040 8bytes 01 00 00 00 00 00 00 00
│   ├── 0x2058 8bytes 02 00 00 00 00 00 00 00
│   ├── 0x2070 8bytes 03 00 00 00 00 00 00 00
│   ├── 0x2088 8bytes 04 00 00 00 00 00 00 00
│   ├── 0x20A0 8bytes 05 00 00 00 00 00 00 00
│   ├── 0x20B8 8bytes 06 00 00 00 00 00 00 00
├──  region2: 8128 bytes
│   ├── 0x4040 8bytes 01 00 00 00 00 00 00 00
│   ├── 0x4058 8bytes 02 00 00 00 00 00 00 00
│   ├── 0x4070 8bytes 03 00 00 00 00 00 00 00
│   ├── 0x4088 8bytes 04 00 00 00 00 00 00 00
│   ├── 0x40A0 8bytes 05 00 00 00 00 00 00 00
├──  region3: 8128 bytes
│   ├── 0x6040 8bytes 01 00 00 00 00 00 00 00
│   ├── 0x6058 8bytes 02 00 00 00 00 00 00 00
│   ├── 0x6070 8bytes 03 00 00 00 00 00 00 00
│   ├── 0x6088 8bytes 04 00 00 00 00 00 00 00
```

For more details please look into [examples](examples/README.md).

## Contact us
For more information about **pmemstream**, contact Igor Chorążewicz (igor.chorazewicz@intel.com),
Piotr Balcer (piotr.balcer@intel.com) or post on our **#pmem** Slack channel using
[this invite link](https://join.slack.com/t/pmem-io/shared_invite/enQtNzU4MzQ2Mzk3MDQwLWQ1YThmODVmMGFkZWI0YTdhODg4ODVhODdhYjg3NmE4N2ViZGI5NTRmZTBiNDYyOGJjYTIyNmZjYzQxODcwNDg)
or [Google group](https://groups.google.com/g/pmem).
