# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(
    NAME analogclocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/analogclocktest.py --failfast
)

add_test(
    NAME appmenutest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/appmenutest.py --failfast
)
set_tests_properties(appmenutest PROPERTIES ENVIRONMENT "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_test(
    NAME calendartest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/calendartest.py --failfast
)

add_test(
    NAME cameraindicatortest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/cameraindicatortest.py
)
set_tests_properties(cameraindicatortest PROPERTIES ENVIRONMENT "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_test(
    NAME clipboardtest_basic
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/clipboardtest.py -k basic
)
add_test(
    NAME clipboardtest_extended
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/clipboardtest.py -k extended
)
add_test(
    NAME clipboarddeadlocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/clipboarddeadlocktest.py
)

set_tests_properties(clipboardtest_basic clipboardtest_extended PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=wayland;GDK_BACKEND=wayland;GSK_RENDERER=cairo")

add_test(
    NAME devicenotifiertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/devicenotifiertest.py --failfast
)

add_test(
    NAME icontest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/icontest.py --failfast
)

add_test(
    NAME lock_logouttest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/lock_logouttest.py --failfast
)

add_test(
    NAME manage-inputmethodtest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/manage-inputmethodtest.py --failfast
)

add_test(
    NAME digitalclocktest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/digitalclocktest.py --failfast
)
set_tests_properties(digitalclocktest PROPERTIES TIMEOUT 120)

add_subdirectory(kicker)

add_test(
    NAME logoutgreetertest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/logoutgreetertest.py
)
set_tests_properties(logoutgreetertest PROPERTIES TIMEOUT 300 ENVIRONMENT "PLASMA_SESSION_GUI_TEST=1;USE_CUSTOM_BUS=1;KDE_INSTALL_FULL_LIBEXECDIR=${KDE_INSTALL_FULL_LIBEXECDIR}")
