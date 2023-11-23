# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_services\")

kcoreaddons_add_plugin(krunner_services SOURCES servicerunner.cpp INSTALL_NAMESPACE "kf6/krunner")
ecm_qt_declare_logging_category(krunner_services
    HEADER debug.h
    IDENTIFIER RUNNER_SERVICES
    CATEGORY_NAME org.kde.plasma.runner.services
    DEFAULT_SEVERITY Warning
    DESCRIPTION "krunner services"
    EXPORT PLASMAWORKSPACE
)
target_link_libraries(krunner_services
    KF6::CoreAddons
    KF6::KIOGui
    KF6::I18n
    KF6::JobWidgets
    KF6::Runner
    KF6::Service
    Plasma::Activities
    Plasma::ActivitiesStats
)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()
