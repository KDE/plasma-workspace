# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME krunnertest
    COMMAND sh -c "mkdir -p /tmp/appium/krunnertest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/krunnertest.py"
)
set_tests_properties(krunnertest PROPERTIES
    TIMEOUT 600
    ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/krunnertest;FLASK_PORT=4750")
