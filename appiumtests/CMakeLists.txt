# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>


if(NOT BUILD_TESTING OR NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()

find_package(SeleniumWebDriverATSPI)
set_package_properties(SeleniumWebDriverATSPI PROPERTIES
    DESCRIPTION "Server component for selenium tests using Linux accessibility infrastructure"
    PURPOSE "Needed for GUI tests"
    URL "https://invent.kde.org/sdk/selenium-webdriver-at-spi"
    TYPE OPTIONAL
)
if(NOT SeleniumWebDriverATSPI_FOUND)
    return()
endif()

if("${QT_MAJOR_VERSION}" STREQUAL "6" AND NOT Qt${QT_MAJOR_VERSION}_VERSION VERSION_GREATER_EQUAL "6.5.0")
    # Before Qt 6.5 the AbstractButtons didn't trigger properly at all.
    return()
endif()

add_test(
    NAME digitalclocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/digitalclocktest.py
)
set_tests_properties(digitalclocktest PROPERTIES TIMEOUT 300)

add_test(
    NAME mediacontrollertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/mediacontrollertest.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(mediacontrollertest PROPERTIES TIMEOUT 300 ENVIRONMENT "LANG=en_US.UTF-8")

add_test(
    NAME systemtraytest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemtraytest.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
# StatusIcon only works with the X11 backend
set_tests_properties(systemtraytest PROPERTIES TIMEOUT 300 ENVIRONMENT "TEST_WITH_XWAYLAND=1;USE_CUSTOM_BUS=1;GDK_BACKEND=x11")

add_test(
    NAME logoutgreetertest
    COMMAND dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/logoutgreetertest.py ${KDE_INSTALL_FULL_LIBEXECDIR}
)
set_tests_properties(logoutgreetertest PROPERTIES TIMEOUT 300 ENVIRONMENT "PLASMA_SESSION_GUI_TEST=1")
