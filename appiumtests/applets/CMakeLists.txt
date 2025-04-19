# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME analogclocktest
    COMMAND sh -c "mkdir -p /tmp/appium/analogclocktest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/analogclocktest.py --failfast"
)
set_tests_properties(analogclocktest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/analogclocktest;FLASK_PORT=4724")

add_test(
    NAME appmenutest
    COMMAND sh -c "mkdir -p /tmp/appium/appmenutest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/appmenutest.py --failfast"
)
set_tests_properties(appmenutest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/appmenutest;FLASK_PORT=4725;TEST_WITH_KWIN_WAYLAND=0;CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_test(
    NAME calendartest
    COMMAND sh -c "mkdir -p /tmp/appium/calendartest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/calendartest.py --failfast"
)
set_tests_properties(calendartest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/calendartest;FLASK_PORT=4726")

add_test(
    NAME cameraindicatortest
    COMMAND sh -c "mkdir -p /tmp/appium/cameraindicatortest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/cameraindicatortest.py"
)
set_tests_properties(cameraindicatortest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/cameraindicatortest;FLASK_PORT=4727;CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_test(
    NAME clipboardtest
    COMMAND sh -c "mkdir -p /tmp/appium/clipboardtest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/clipboardtest.py"
)
set_tests_properties(clipboardtest PROPERTIES TIMEOUT 300 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/clipboardtest;FLASK_PORT=4728;QT_QPA_PLATFORM=wayland;GDK_BACKEND=wayland" RUN_SERIAL ON)

add_test(
    NAME devicenotifiertest
    COMMAND sh -c "mkdir -p /tmp/appium/devicenotifiertest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/devicenotifiertest.py --failfast"
)
set_tests_properties(devicenotifiertest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/devicenotifiertest;FLASK_PORT=4729")

add_test(
    NAME icontest
    COMMAND sh -c "mkdir -p /tmp/appium/icontest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/icontest.py --failfast"
)
set_tests_properties(icontest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/icontest;FLASK_PORT=4730")

add_test(
    NAME lock_logouttest
    COMMAND sh -c "mkdir -p /tmp/appium/lock_logouttest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/lock_logouttest.py --failfast"
)
set_tests_properties(lock_logouttest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/lock_logouttest;FLASK_PORT=4731")

add_test(
    NAME manage-inputmethodtest
    COMMAND sh -c "mkdir -p /tmp/appium/inputmethodtest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/manage-inputmethodtest.py --failfast"
)
set_tests_properties(manage-inputmethodtest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/inputmethodtest;FLASK_PORT=4732")

add_test(
    NAME notificationstest
    COMMAND sh -c "mkdir -p /tmp/appium/notificationstest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/notificationstest.py"
)
set_tests_properties(notificationstest PROPERTIES TIMEOUT 300 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/notificationstest;FLASK_PORT=4733;KACTIVITYMANAGERD_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/kactivitymanagerd" RUN_SERIAL ON)

add_test(
    NAME digitalclocktest
    COMMAND sh -c "mkdir -p /tmp/appium/digitalclocktest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/digitalclocktest.py --failfast"
)
set_tests_properties(digitalclocktest PROPERTIES TIMEOUT 300 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/digitalclocktest;FLASK_PORT=4734")

add_subdirectory(kicker)

add_test(
    NAME mediacontrollertest
    COMMAND sh -c "mkdir -p /tmp/appium/mediacontrollertest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/mediacontrollertest.py --failfast"
)
set_tests_properties(mediacontrollertest PROPERTIES TIMEOUT 300 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/mediacontrollertest;FLASK_PORT=4735;GDK_BACKEND=wayland")

add_test(
    NAME systemtraytest_x11
    COMMAND dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemtraytest.py --failfast
)
# StatusIcon only works with the X11 backend
set_tests_properties(systemtraytest_x11 PROPERTIES TIMEOUT 120 ENVIRONMENT "TEST_WITH_KWIN_WAYLAND=0;GDK_BACKEND=x11;CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" RUN_SERIAL ON)

add_test(
    NAME systemtraytest_wayland
    COMMAND dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/systemtraytest.py --failfast
)
set_tests_properties(systemtraytest_wayland PROPERTIES TIMEOUT 120 ENVIRONMENT "TEST_WITH_XWAYLAND=1;GDK_BACKEND=x11;QT_QPA_PLATFORM=wayland;CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" RUN_SERIAL ON)

add_test(
    NAME logoutgreetertest
    COMMAND sh -c "mkdir -p /tmp/appium/logoutgreetertest;dbus-launch selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/logoutgreetertest.py"
)
set_tests_properties(logoutgreetertest PROPERTIES TIMEOUT 120 ENVIRONMENT "XDG_RUNTIME_DIR=/tmp/appium/logoutgreetertest;FLASK_PORT=4736;PLASMA_SESSION_GUI_TEST=1;KDE_INSTALL_FULL_LIBEXECDIR=${KDE_INSTALL_FULL_LIBEXECDIR}")
