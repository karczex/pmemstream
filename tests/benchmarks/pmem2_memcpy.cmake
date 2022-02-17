# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022, Intel Corporation

include(${TESTS_ROOT_DIR}/cmake/exec_functions.cmake)

setup()

execute(${EXECUTABLE} --path ${DIR}/testfile --flags "PMEM2_F_MEM_NODRAIN" --persist)

finish()
