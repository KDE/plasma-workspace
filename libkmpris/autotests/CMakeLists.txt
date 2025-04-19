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
    COMMAND dbus-launch python3 ${CMAKE_CURRENT_SOURCE_DIR}/mprisdeclarativetest.py "tst_Mpris2Model.qml"
)
add_test(
    NAME mprisdeclarativetest_MultiplexerModel
    COMMAND dbus-launch python3 ${CMAKE_CURRENT_SOURCE_DIR}/mprisdeclarativetest.py "tst_MultiplexerModel.qml"
)
execute_process(COMMAND gcc -print-file-name=libasan.so OUTPUT_VARIABLE LIBASAN_PATH ECHO_OUTPUT_VARIABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
set_tests_properties(
    mprisdeclarativetest_Mpris2Model
    mprisdeclarativetest_MultiplexerModel
    PROPERTIES
        ENVIRONMENT "LC_ALL=en_US.UTF-8;LD_PRELOAD=${LIBASAN_PATH};CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin;QMLTEST_PATH=$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>"
)

find_program(xdotool_PATH xdotool REQUIRED)
find_program(kglobalacceld_PATH kglobalacceld PATHS ${KDE_INSTALL_FULL_LIBEXECDIR} REQUIRED)
add_test(NAME mediakeystest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/mediakeystest.py ${KDE_INSTALL_FULL_LIBEXECDIR}/kglobalacceld
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../appiumtests
)
set_tests_properties(mediakeystest PROPERTIES RUN_SERIAL ON)
