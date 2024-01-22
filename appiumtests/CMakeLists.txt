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
if(NOT SeleniumWebDriverATSPI_FOUND AND NOT DEFINED ENV{KDECI_BUILD})
    return()
endif()

add_test(
    NAME analogclocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/analogclocktest.py
)
set_tests_properties(analogclocktest PROPERTIES TIMEOUT 120)

add_test(
    NAME appmenutest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/appmenutest.py
)
set_tests_properties(appmenutest PROPERTIES TIMEOUT 120)

add_test(
    NAME batterymonitortest_withdisplaydevice
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/batterymonitortest.py --failfast
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(batterymonitortest_withdisplaydevice PROPERTIES TIMEOUT 120 ENVIRONMENT "ENABLE_DISPLAY_DEVICE=0;POWERDEVIL_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/org_kde_powerdevil")

add_test(
    NAME batterymonitortest_withoutdisplaydevice
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/batterymonitortest.py --failfast
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(batterymonitortest_withoutdisplaydevice PROPERTIES TIMEOUT 120 ENVIRONMENT "ENABLE_DISPLAY_DEVICE=0;POWERDEVIL_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/org_kde_powerdevil")

add_test(
    NAME calendartest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/calendartest.py
)
set_tests_properties(calendartest PROPERTIES TIMEOUT 120)

add_test(
    NAME clipboardtest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/clipboardtest.py --failfast
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(clipboardtest PROPERTIES TIMEOUT 300)

add_test(
    NAME devicenotifiertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/devicenotifiertest.py
)
set_tests_properties(devicenotifiertest PROPERTIES TIMEOUT 120)

add_test(
    NAME icontest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/icontest.py
)
set_tests_properties(icontest PROPERTIES TIMEOUT 120)

add_test(
    NAME lock_logouttest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/lock_logouttest.py
)
set_tests_properties(lock_logouttest PROPERTIES TIMEOUT 120)

add_test(
    NAME manage-inputmethodtest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/manage-inputmethodtest.py
)
set_tests_properties(manage-inputmethodtest PROPERTIES TIMEOUT 120)

add_test(
    NAME notificationstest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/notificationstest.py
)
set_tests_properties(notificationstest PROPERTIES TIMEOUT 120)

add_test(
    NAME digitalclocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/digitalclocktest.py
)
set_tests_properties(digitalclocktest PROPERTIES TIMEOUT 300)

add_test(
    NAME mediacontrollertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/mediacontrollertest.py --failfast
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(mediacontrollertest PROPERTIES TIMEOUT 300 ENVIRONMENT "GDK_BACKEND=wayland")

add_test(
    NAME systemtraytest_x11
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemtraytest.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
# StatusIcon only works with the X11 backend
set_tests_properties(systemtraytest_x11 PROPERTIES TIMEOUT 120 ENVIRONMENT "TEST_WITH_KWIN_WAYLAND=0;USE_CUSTOM_BUS=1;GDK_BACKEND=x11")

add_test(
    NAME systemtraytest_wayland
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemtraytest.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(systemtraytest_wayland PROPERTIES TIMEOUT 120 ENVIRONMENT "TEST_WITH_XWAYLAND=1;USE_CUSTOM_BUS=1;GDK_BACKEND=x11")

add_test(
    NAME logoutgreetertest
    COMMAND dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/logoutgreetertest.py
)
set_tests_properties(logoutgreetertest PROPERTIES TIMEOUT 300 ENVIRONMENT "PLASMA_SESSION_GUI_TEST=1;KDE_INSTALL_FULL_LIBEXECDIR=${KDE_INSTALL_FULL_LIBEXECDIR}")

add_test(
    NAME cameraindicatortest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/cameraindicatortest.py
)
set_tests_properties(cameraindicatortest PROPERTIES TIMEOUT 120)

# KCM autotests
add_test(
    NAME kcm_autostart_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_autostart_test.py
)
set_tests_properties(kcm_autostart_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_colors_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_colors_test.py
)
set_tests_properties(kcm_colors_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_cursortheme_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_cursortheme_test.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(kcm_cursortheme_test PROPERTIES TIMEOUT 120 ENVIRONMENT "TEST_WITH_KWIN_WAYLAND=0")

add_test(
    NAME kcm_desktoptheme_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_desktoptheme_test.py
)
set_tests_properties(kcm_desktoptheme_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_feedback_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_feedback_test.py
)
set_tests_properties(kcm_feedback_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_fonts_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_fonts_test.py
)
set_tests_properties(kcm_fonts_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_icons_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_icons_test.py
)
set_tests_properties(kcm_icons_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_fontinst_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_fontinst_test.py
)
set_tests_properties(kcm_fontinst_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_lookandfeel_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_lookandfeel_test.py
)
set_tests_properties(kcm_lookandfeel_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_nightcolor_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_nightcolor_test.py
)
set_tests_properties(kcm_nightcolor_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_notifications_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_notifications_test.py
)
set_tests_properties(kcm_notifications_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_regionandlang_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_regionandlang_test.py
)
set_tests_properties(kcm_regionandlang_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_soundtheme_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_soundtheme_test.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(kcm_soundtheme_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_style_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_style_test.py
)
set_tests_properties(kcm_style_test PROPERTIES TIMEOUT 120)

add_test(
    NAME kcm_users_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_users_test.py --failfast
)
set_tests_properties(kcm_users_test PROPERTIES TIMEOUT 120)
