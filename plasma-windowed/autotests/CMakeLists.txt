# SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

# QML module providing PlasmoidTestItem, the test harness that reuses
# PlasmaWindowedCorona to host an applet inside a qmltestrunner scene.
ecm_add_qml_module(plasmoidtestplugin
    URI "org.kde.plasmoidtest"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtQuick
)

target_sources(plasmoidtestplugin PRIVATE
    plasmoidtestby.h
    plasmoidtestitem.cpp
    plasmoidtestitem.h
)

target_link_libraries(plasmoidtestplugin PRIVATE
    Qt::Quick
    Qt::Qml
    Plasma::Plasma
    Plasma::PlasmaQuick
    plasmawindowedcore
)

# Private QtQuick headers for the QQuickAccessibleAttached fallback used by
# findItem() when the public QAccessible interface is unavailable.
if(TARGET Qt6::QuickPrivate)
    target_link_libraries(plasmoidtestplugin PRIVATE Qt6::QuickPrivate)
endif()

# Intentionally installed into ${KDE_INSTALL_QMLDIR}: with BUILD_TESTING=ON the
# org.kde.plasmoidtest module is part of the shared test infrastructure and other
# KDE repos' tests import it from the install prefix. autotests/ is only added
# under if(BUILD_TESTING), so this only installs in test builds.
ecm_finalize_qml_module(plasmoidtestplugin)

find_package(Qt6 REQUIRED COMPONENTS QuickTest)

set(_qmltestrunner $<TARGET_FILE:Qt::qmltestrunner>)

# offscreen is enough for loadApplet + findItem (Accessible) assertions;
# mouse/keyboard interaction tests should switch QT_QPA_PLATFORM to wayland
# under a nested kwin (see AGENTS.md: never touch the session compositor).
# LC_ALL=en_US.UTF-8 matches the appiumtests, so Accessible.name is English.
# The built QML module lands at <builddir>/bin/org/kde/plasmoidtest, so the
# import path is <builddir>/bin (not bin/qml).
set(_plasmoid_test_environment
    "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;QML2_IMPORT_PATH=${CMAKE_BINARY_DIR}/bin;QT_LOGGING_RULES=qt.accessibility.atspi.warning=false"
)

set(_plasmoid_qml_tests
    by_test
    icon_test
    lock_logout_test
    manage_inputmethod_test
    calendar_test
    analogclock_test
    appmenu_test
    cameraindicator_test
    devicenotifier_test
)

foreach(_test ${_plasmoid_qml_tests})
    add_test(
        NAME plasmoidtest_${_test}
        COMMAND ${_qmltestrunner}
                -input ${CMAKE_CURRENT_SOURCE_DIR}/${_test}.qml
    )
    set_tests_properties(plasmoidtest_${_test} PROPERTIES
        TIMEOUT 120
        ENVIRONMENT "${_plasmoid_test_environment}"
    )
endforeach()
