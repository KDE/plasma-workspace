# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME ksplashtest_wayland
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/ksplashtest.py --failfast
)
set_tests_properties(ksplashtest_wayland PROPERTIES TIMEOUT 60)

add_test(
    NAME ksplashtest_x11
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/ksplashtest.py --failfast
)
set_tests_properties(ksplashtest_x11 PROPERTIES TIMEOUT 60 ENVIRONMENT "TEST_WITH_KWIN_WAYLAND=0")
