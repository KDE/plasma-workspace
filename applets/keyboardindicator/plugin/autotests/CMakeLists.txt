# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_test(keystatetest.cpp LINK_LIBRARIES Qt::Test Qt::Qml Qt::Quick)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()

add_test(
    NAME lockedtest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/lockedtest.py
)
set_tests_properties(lockedtest PROPERTIES
    TIMEOUT 60
    ENVIRONMENT "TEST_WITH_KWIN_WAYLAND=0;QML_EXEC=$<TARGET_FILE_DIR:Qt${QT_MAJOR_VERSION}::qmake>/qml")
