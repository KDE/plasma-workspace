# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_sessions\")

set(krunner_sessions_SRCS
    sessionrunner.cpp sessionrunner.h
)

qt_add_dbus_interface(krunner_sessions_SRCS ${SCREENSAVER_DBUS_INTERFACE} screensaver_interface)

kcoreaddons_add_plugin(krunner_sessions SOURCES ${krunner_sessions_SRCS} INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_sessions Qt::DBus KF6::Runner KF6::I18n PW::KWorkspace)
