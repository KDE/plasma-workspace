# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME ksplashtest_wayland
    COMMAND sh -c "mkdir -p /tmp/appium/ksplashtest_wayland;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/ksplashtest.py --failfast"
)
set_tests_properties(ksplashtest_wayland PROPERTIES TIMEOUT 60 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/ksplashtest_wayland;FLASK_PORT=5701")

add_test(
    NAME ksplashtest_x11
    COMMAND sh -c "mkdir -p /tmp/appium/ksplashtest_x11;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/ksplashtest.py --failfast"
)
set_tests_properties(ksplashtest_x11 PROPERTIES TIMEOUT 60 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/ksplashtest_x11;FLASK_PORT=5702;TEST_WITH_KWIN_WAYLAND=0")
