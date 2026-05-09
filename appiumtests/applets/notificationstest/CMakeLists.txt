# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set(notificationstest_env "KACTIVITYMANAGERD_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/kactivitymanagerd;USE_CUSTOM_BUS=1")

add_test(
    NAME notificationstest_malformed_image
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_1_malformed_image.py --failfast
)
set_tests_properties(notificationstest_malformed_image PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_image_notification
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_2_image_notification.py --failfast
)
set_tests_properties(notificationstest_image_notification PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_explicit_timeout
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_3_explicit_timeout.py --failfast
)
set_tests_properties(notificationstest_explicit_timeout PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_html_to_plaintext
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_4_html_to_plaintext.py --failfast
)
set_tests_properties(notificationstest_html_to_plaintext PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_actions
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_5_actions.py --failfast
)
set_tests_properties(notificationstest_actions PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_inline_reply
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_6_inline_reply.py --failfast
)
set_tests_properties(notificationstest_inline_reply PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_thumbnail
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_7_thumbnail.py --failfast
)
set_tests_properties(notificationstest_thumbnail PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_job_notification
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_8_job_notification.py --failfast
)
set_tests_properties(notificationstest_job_notification PROPERTIES ENVIRONMENT "${notificationstest_env}")

add_test(
    NAME notificationstest_do_not_disturb
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/test_9_do_not_disturb.py --failfast
)
set_tests_properties(notificationstest_do_not_disturb PROPERTIES ENVIRONMENT "${notificationstest_env}")
