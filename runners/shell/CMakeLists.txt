# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_shell\")

kcoreaddons_add_plugin(krunner_shell SOURCES shellrunner.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_shell
    KF6::KIOCore
    KF6::KIOGui
    KF6::I18n
    KF6::Notifications
    KF6::Plasma
    KF6::Runner
    KF6::Completion
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

