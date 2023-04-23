# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_recentdocuments\")

kcoreaddons_add_plugin(krunner_recentdocuments SOURCES recentdocuments.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_recentdocuments
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::I18n
    KF6::ActivitiesStats
    KF6::Runner
    KF6::Notifications
)
