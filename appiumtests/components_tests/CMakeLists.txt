# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME systemdialogtest
    COMMAND sh -c "mkdir -p /tmp/appium/systemdialogtest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemdialogtest.py"
)
execute_process(COMMAND gcc -print-file-name=libasan.so OUTPUT_VARIABLE LIBASAN_PATH ECHO_OUTPUT_VARIABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
set_tests_properties(systemdialogtest PROPERTIES
    TIMEOUT 60
    ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/systemdialogtest;FLASK_PORT=4738;LD_PRELOAD=${LIBASAN_PATH};QML_EXEC=$<TARGET_FILE_DIR:Qt${QT_MAJOR_VERSION}::qmake>/qml")
