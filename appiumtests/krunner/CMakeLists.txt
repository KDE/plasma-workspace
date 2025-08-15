# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME krunnertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/krunnertest.py
)
set_tests_properties(krunnertest PROPERTIES
    TIMEOUT 600
    ENVIRONMENT "USE_CUSTOM_BUS=1")
