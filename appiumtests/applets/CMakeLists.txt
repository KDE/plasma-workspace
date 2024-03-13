# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME analogclocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/analogclocktest.py --failfast
)
set_tests_properties(analogclocktest PROPERTIES TIMEOUT 120)

add_test(
    NAME appmenutest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/appmenutest.py --failfast
)
set_tests_properties(appmenutest PROPERTIES TIMEOUT 120)

add_test(
    NAME batterymonitortest_withdisplaydevice
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/batterymonitortest.py --failfast
)
set_tests_properties(batterymonitortest_withdisplaydevice PROPERTIES TIMEOUT 120 ENVIRONMENT "ENABLE_DISPLAY_DEVICE=0;POWERDEVIL_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/org_kde_powerdevil")

add_test(
    NAME batterymonitortest_withoutdisplaydevice
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/batterymonitortest.py --failfast
)
set_tests_properties(batterymonitortest_withoutdisplaydevice PROPERTIES TIMEOUT 120 ENVIRONMENT "ENABLE_DISPLAY_DEVICE=0;POWERDEVIL_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/org_kde_powerdevil")

add_test(
    NAME calendartest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/calendartest.py --failfast
)
set_tests_properties(calendartest PROPERTIES TIMEOUT 120)

add_test(
    NAME clipboardtest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/clipboardtest.py --failfast
)
set_tests_properties(clipboardtest PROPERTIES TIMEOUT 300)

add_test(
    NAME devicenotifiertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/devicenotifiertest.py --failfast
)
set_tests_properties(devicenotifiertest PROPERTIES TIMEOUT 120)

add_test(
    NAME icontest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/icontest.py --failfast
)
set_tests_properties(icontest PROPERTIES TIMEOUT 120)

add_test(
    NAME lock_logouttest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/lock_logouttest.py --failfast
)
set_tests_properties(lock_logouttest PROPERTIES TIMEOUT 120)

add_test(
    NAME manage-inputmethodtest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/manage-inputmethodtest.py --failfast
)
set_tests_properties(manage-inputmethodtest PROPERTIES TIMEOUT 120)

add_test(
    NAME notificationstest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/notificationstest.py --failfast
)
set_tests_properties(notificationstest PROPERTIES TIMEOUT 120)

add_test(
    NAME digitalclocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/digitalclocktest.py --failfast
)
set_tests_properties(digitalclocktest PROPERTIES TIMEOUT 300)

add_test(
    NAME mediacontrollertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/mediacontrollertest.py --failfast
)
set_tests_properties(mediacontrollertest PROPERTIES TIMEOUT 300 ENVIRONMENT "GDK_BACKEND=wayland")

add_test(
    NAME systemtraytest_x11
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemtraytest.py --failfast
)
# StatusIcon only works with the X11 backend
set_tests_properties(systemtraytest_x11 PROPERTIES TIMEOUT 120 ENVIRONMENT "TEST_WITH_KWIN_WAYLAND=0;USE_CUSTOM_BUS=1;GDK_BACKEND=x11")

add_test(
    NAME systemtraytest_wayland
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemtraytest.py
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
