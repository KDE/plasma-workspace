# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME wallpaperinstalltest
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/installtest.py --failfast
)
set_tests_properties(wallpaperinstalltest PROPERTIES TIMEOUT 600 ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin")
