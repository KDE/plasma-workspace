# SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

plasma_add_applet(org.kde.plasma.activitybar
    QML_SOURCES
        qml/main.qml
    GENERATE_APPLET_CLASS
)

add_test(NAME applet_activitybar_smoketest COMMAND plasmawindowed --smoke-test org.kde.plasma.activitybar)
set_tests_properties(applet_activitybar_smoketest PROPERTIES
    ENVIRONMENT_MODIFICATION QT_PLUGIN_PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin
)
