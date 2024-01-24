# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN="plasma_runner_appstream")

kcoreaddons_add_plugin(krunner_appstream SOURCES appstreamrunner.cpp INSTALL_NAMESPACE "kf6/krunner")

ecm_qt_declare_logging_category(krunner_appstream
    HEADER debug.h
    IDENTIFIER RUNNER_APPSTREAM
    CATEGORY_NAME org.kde.plasma.runner.appstream
    DEFAULT_SEVERITY Warning
    DESCRIPTION "krunner appstream"
    EXPORT PLASMAWORKSPACE
)
target_link_libraries(krunner_appstream PUBLIC Qt::Gui KF6::Runner KF6::I18n KF6::Service KF6::KIOGui KF6::JobWidgets AppStreamQt)
