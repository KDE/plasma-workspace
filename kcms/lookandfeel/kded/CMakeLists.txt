# SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

kcoreaddons_add_plugin(lookandfeelautoswitcher INSTALL_NAMESPACE "kf6/kded")

target_sources(lookandfeelautoswitcher PRIVATE
    idletimeout.cpp
    lookandfeelautoswitcher.cpp
)

kconfig_target_kcfg_file(lookandfeelautoswitcher
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/../lookandfeelsettings.kcfg
    CLASS_NAME LookAndFeelSettings
)

kconfig_target_kcfg_file(lookandfeelautoswitcher
    FILE lookandfeelautoswitcherstate.kcfg
    CLASS_NAME LookAndFeelAutoSwitcherState
    MUTATORS
)

target_link_libraries(lookandfeelautoswitcher PRIVATE
    Qt6::Core
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::IdleTime
    KNightTime
)
