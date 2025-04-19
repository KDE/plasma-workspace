# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

add_test(
    NAME kicker_favoritetest
    COMMAND sh -c "mkdir -p /tmp/appium/kicker;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/favoritetest.py --failfast"
)
execute_process(COMMAND gcc -print-file-name=libasan.so OUTPUT_VARIABLE LIBASAN_PATH ECHO_OUTPUT_VARIABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
set_tests_properties(kicker_favoritetest PROPERTIES
    TIMEOUT 120
    ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/kicker;FLASK_PORT=4737;LD_PRELOAD=${LIBASAN_PATH};KACTIVITYMANAGERD_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/kactivitymanagerd;QMLTEST_EXEC=$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>")
