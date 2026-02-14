# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

ecm_add_test(fetchinitialplayertest.cpp LINK_LIBRARIES kmpris Qt::Test)
set_tests_properties(
    fetchinitialplayertest
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME mprisdeclarativetest_Mpris2Model
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/mprisdeclarativetest.py "tst_Mpris2Model.qml"
)
add_test(
    NAME mprisdeclarativetest_MultiplexerModel
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/mprisdeclarativetest.py "tst_MultiplexerModel.qml"
)
set_tests_properties(
    mprisdeclarativetest_Mpris2Model
    mprisdeclarativetest_MultiplexerModel
    PROPERTIES
        ENVIRONMENT "LC_ALL=en_US.UTF-8;CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin;QMLTEST_PATH=$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>"
)
