# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_test(dbusservicewatchertest.cpp LINK_LIBRARIES Qt::Test Qt::Qml Qt::Quick)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()

add_test(
    NAME dbussignalwatchertest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dbustestwrapper.py "dbussignalwatchertest.qml"
)
add_test(
    NAME dbusmethodcalltest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dbustestwrapper.py "dbusmethodcalltest.qml"
)
add_test(
    NAME dbuspropertiestest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/dbustestwrapper.py "dbuspropertiestest.qml"
)
set_tests_properties(dbussignalwatchertest dbusmethodcalltest dbuspropertiestest PROPERTIES ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin;QMLTEST_PATH=$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>")
