# SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_subdirectory(x11)

kcoreaddons_add_plugin(appmenu SOURCES appmenustub.cpp INSTALL_NAMESPACE "kf6/kded")

target_link_libraries(appmenu
    Qt::DBus
    KF6::CoreAddons
    KF6::DBusAddons
)
