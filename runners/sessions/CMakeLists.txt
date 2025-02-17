# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_sessions\")

set(krunner_sessions_SRCS
    sessionrunner.cpp sessionrunner.h
)

qt_add_dbus_adaptor(krunner_sessions_SRCS "org.kde.krunner1.xml" sessionrunner.h SessionRunner)

qt_add_dbus_interface(krunner_sessions_SRCS ${SCREENSAVER_DBUS_INTERFACE} screensaver_interface)

add_executable(krunner_sessions ${krunner_sessions_SRCS})

target_link_libraries(krunner_sessions
    Qt::Widgets
    Qt::DBus
    KF6::Runner
    KF6::I18n
    KF6::ConfigCore
    KF6::WidgetsAddons
    KF6::Crash
    PW::KWorkspace)

install(TARGETS krunner_sessions DESTINATION ${KDE_INSTALL_LIBEXECDIR})

install(
  FILES plasma-runner-sessions.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/krunner/dbusplugins"
)

ecm_generate_dbus_service_file(NAME org.kde.runners.sessions
    EXECUTABLE ${KDE_INSTALL_FULL_LIBEXECDIR}/krunner_sessions
    SYSTEMD_SERVICE plasma-runner-sessions.service
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

ecm_install_configured_files(INPUT plasma-runner-sessions.service.in @ONLY DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
