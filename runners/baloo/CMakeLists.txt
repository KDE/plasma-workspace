# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_baloosearch5\")

set(baloosearchrunner_SRCS
    baloosearchrunner.cpp
    baloosearchrunner.h)

qt_add_dbus_adaptor(baloosearchrunner_SRCS "org.kde.krunner1.xml" baloosearchrunner.h SearchRunner)
add_executable(baloorunner ${baloosearchrunner_SRCS})

target_link_libraries(baloorunner
  KF6::Runner
  KF6::KIOWidgets
  KF6::I18n
  KF6::Baloo
  KF6::Notifications
  Qt::DBus
)

install(TARGETS baloorunner DESTINATION ${KDE_INSTALL_LIBEXECDIR})

install(
  FILES plasma-runner-baloosearch.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/krunner/dbusplugins"
)

ecm_generate_dbus_service_file(NAME org.kde.runners.baloo
    EXECUTABLE ${KDE_INSTALL_FULL_LIBEXECDIR}/baloorunner
    SYSTEMD_SERVICE plasma-baloorunner.service
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

ecm_install_configured_files(INPUT plasma-baloorunner.service.in @ONLY DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
