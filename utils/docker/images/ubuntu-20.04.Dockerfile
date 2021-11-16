# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

#
# Dockerfile - a 'recipe' for Docker to build an image of ubuntu-based
#              environment prepared for running pmemstream tests.
#

# Pull base image
FROM ghcr.io/pmem/libpmemobj-cpp:ubuntu-20.04-latest
MAINTAINER igor.chorazewicz@intel.com

USER root

RUN dpkg -i /opt/pmdk-pkg/*.deb

# Update the apt cache and install basic tools
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
	txt2man \
 && rm -rf /var/lib/apt/lists/*

