# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_locations\")

kcoreaddons_add_plugin(locations SOURCES locationrunner.h locationrunner.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(locations
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::I18n
    KF6::Runner
    KF6::Notifications
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
