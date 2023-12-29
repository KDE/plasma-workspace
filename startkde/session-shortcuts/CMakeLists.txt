# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

add_definitions(-DTRANSLATION_DOMAIN=\"session-shortcuts-kded\")

kcoreaddons_add_plugin(plasma-session-shortcuts SOURCES main.cpp INSTALL_NAMESPACE "kf6/kded")

target_link_libraries(plasma-session-shortcuts
   Qt::Core
   Qt::DBus
   KF6::GlobalAccel
   KF6::XmlGui
   KF6::I18n
   KF6::DBusAddons # For KDEDModule
   PW::KWorkspace
)
