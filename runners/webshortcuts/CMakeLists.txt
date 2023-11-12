# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_webshortcuts\")

kcoreaddons_add_plugin(krunner_webshortcuts SOURCES webshortcutrunner.cpp webshortcutrunner.h INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_webshortcuts
    Qt::DBus
    KF6::Runner
    KF6::Service
    KF6::KIOWidgets
    KF6::I18n
    PW::KWorkspace
)
