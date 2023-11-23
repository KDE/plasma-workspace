# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_powerdevil\")

kcoreaddons_add_plugin(krunner_powerdevil SOURCES PowerDevilRunner.h PowerDevilRunner.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_powerdevil Qt::DBus KF6::ConfigCore KF6::I18n Plasma::Plasma KF6::Runner PW::KWorkspace)
