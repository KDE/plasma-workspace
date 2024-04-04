# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

add_definitions(-DTRANSLATION_DOMAIN=\"oom-notifier\")

kcoreaddons_add_plugin(oom-notifier SOURCES module.cpp INSTALL_NAMESPACE "kf6/kded")
target_link_libraries(oom-notifier
    KF6::DBusAddons
    KF6::I18n
    KF6::Notifications
    KF6::CoreAddons
    KF6::Service
    KF6::KIOGui
)

add_subdirectory(autotests)
