# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

ecm_add_test(fetchinitialplayertest.cpp LINK_LIBRARIES kmpris Qt::Test)
ecm_add_test(mprisdeclarativetest.cpp LINK_LIBRARIES kmpris Qt::Test Qt::Quick Qt::Qml)
set_tests_properties(
    fetchinitialplayertest
    mprisdeclarativetest
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

find_program(xdotool_PATH xdotool REQUIRED)
find_program(kglobalacceld_PATH kglobalacceld PATHS ${KDE_INSTALL_FULL_LIBEXECDIR} REQUIRED)
add_test(NAME mediakeystest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/mediakeystest.py ${KDE_INSTALL_FULL_LIBEXECDIR}/kglobalacceld
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../appiumtests
)
