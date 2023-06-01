# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_services\")

add_library(krunner_services_static STATIC servicerunner.cpp)
set_property(TARGET krunner_services_static PROPERTY POSITION_INDEPENDENT_CODE ON)

ecm_qt_declare_logging_category(krunner_services_static
    HEADER debug.h
    IDENTIFIER RUNNER_SERVICES
    CATEGORY_NAME org.kde.plasma.runner.services
    DEFAULT_SEVERITY Warning)

target_link_libraries(krunner_services_static
    KF6::CoreAddons
    KF6::KIOGui
    KF6::I18n
    KF6::Notifications
    KF6::Runner
    KF6::Service
    KF6::Activities
    KF6::ActivitiesStats
)

kcoreaddons_add_plugin(krunner_services SOURCES plugin.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_services krunner_services_static)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()
