# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_test(dbusservicewatchertest.cpp LINK_LIBRARIES Qt::Test Qt::Qml Qt::Quick)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()

add_test(
    NAME dbusmethodcalltest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dbusmethodcalltest.py
)
execute_process(COMMAND gcc -print-file-name=libasan.so OUTPUT_VARIABLE LIBASAN_PATH ECHO_OUTPUT_VARIABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
set_tests_properties(dbusmethodcalltest PROPERTIES ENVIRONMENT "LD_PRELOAD=${LIBASAN_PATH};CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin;QMLTEST_PATH=$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>")
