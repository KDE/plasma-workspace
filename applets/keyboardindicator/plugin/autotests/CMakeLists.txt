# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()

add_test(
    NAME lockedtest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/lockedtest.py
)
execute_process(COMMAND gcc -print-file-name=libasan.so OUTPUT_VARIABLE LIBASAN_PATH ECHO_OUTPUT_VARIABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
set_tests_properties(lockedtest PROPERTIES
    TIMEOUT 60
    ENVIRONMENT "LD_PRELOAD=${LIBASAN_PATH};TEST_WITH_KWIN_WAYLAND=0;QML_EXEC=$<TARGET_FILE_DIR:Qt${QT_MAJOR_VERSION}::qmake>/qml")
