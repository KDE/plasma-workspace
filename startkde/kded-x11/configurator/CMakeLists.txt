# SPDX-FileCopyrightText: 2026 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

kcoreaddons_add_plugin(configurator INSTALL_NAMESPACE "kf6/kded-x11")

target_sources(configurator PRIVATE
    configurator.cpp
)

target_link_libraries(configurator PRIVATE
    Qt6::DBus
    Qt6::Gui
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::DBusAddons
    XCB::XCB
    XCB::CURSOR
)
