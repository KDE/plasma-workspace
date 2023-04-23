# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_kill\")

kcoreaddons_add_plugin(kcm_krunner_kill SOURCES killrunner_config.h killrunner_config.cpp INSTALL_NAMESPACE "kf6/krunner/kcms")
target_compile_definitions(kcm_krunner_kill PRIVATE -DKRUNNER_PLUGIN_NAME="krunner_kill")
ki18n_wrap_ui(kcm_krunner_kill killrunner_config.ui)
target_link_libraries(kcm_krunner_kill
    Qt::Core
    Qt::Gui
    KF6::CoreAddons
    KF6::KCMUtils
    KF6::I18n
    KF6::Completion
    KF6::Runner
)

kcoreaddons_add_plugin(krunner_kill SOURCES killrunner.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_kill
    KF6::I18n
    KF6::ConfigCore
    KF6::Completion
    KF6::AuthCore
    KF6::Runner
    KSysGuard::ProcessCore
)
