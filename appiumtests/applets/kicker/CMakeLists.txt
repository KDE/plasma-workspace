# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

add_test(
    NAME kicker_favoritetest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/favoritetest.py --failfast
)
set_tests_properties(kicker_favoritetest PROPERTIES
    TIMEOUT 120
    ENVIRONMENT "USE_CUSTOM_BUS=1;KACTIVITYMANAGERD_PATH=${KDE_INSTALL_FULL_LIBEXECDIR}/kactivitymanagerd;QMLTEST_EXEC=$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>")
