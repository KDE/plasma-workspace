# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set(mediacontrollertest_env "USE_CUSTOM_BUS=1")

add_test(
    NAME mediacontrollertest_basic_playback
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_1_basic_playback.py --failfast
)
set_tests_properties(mediacontrollertest_basic_playback PROPERTIES ENVIRONMENT "${mediacontrollertest_env}")

add_test(
    NAME mediacontrollertest_playback_controls
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_2_playback_controls.py --failfast
)
set_tests_properties(mediacontrollertest_playback_controls PROPERTIES ENVIRONMENT "${mediacontrollertest_env}")

add_test(
    NAME mediacontrollertest_multiple_players
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_3_multiple_players.py --failfast
)
set_tests_properties(mediacontrollertest_multiple_players PROPERTIES ENVIRONMENT "${mediacontrollertest_env}")

add_test(
    NAME mediacontrollertest_browser_player
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_4_browser_player.py --failfast
)
set_tests_properties(mediacontrollertest_browser_player PROPERTIES ENVIRONMENT "${mediacontrollertest_env}")

add_test(
    NAME mediacontrollertest_plasma_browser_integration
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_5_plasma_browser_integration.py --failfast
)
set_tests_properties(mediacontrollertest_plasma_browser_integration PROPERTIES ENVIRONMENT "${mediacontrollertest_env}")

add_test(
    NAME mediacontrollertest_encoded_url
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_6_encoded_url.py --failfast
)
set_tests_properties(mediacontrollertest_encoded_url PROPERTIES ENVIRONMENT "${mediacontrollertest_env}")

add_test(
    NAME mediacontrollertest_invalid_player
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_7_invalid_player.py --failfast
)
set_tests_properties(mediacontrollertest_invalid_player PROPERTIES ENVIRONMENT "${mediacontrollertest_env}")