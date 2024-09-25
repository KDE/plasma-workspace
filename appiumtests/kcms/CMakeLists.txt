# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

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
    NAME kcm_nightlight_test
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kcm_nightlight_test.py
)
set_tests_properties(kcm_nightlight_test PROPERTIES TIMEOUT 120)

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
