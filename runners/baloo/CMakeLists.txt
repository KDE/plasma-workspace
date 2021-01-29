add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_baloosearch5\")


set(baloosearchrunner_SRCS
    baloosearchrunner.cpp)

qt5_add_dbus_adaptor(baloosearchrunner_SRCS "org.kde.krunner1.xml" baloosearchrunner.h SearchRunner)
add_executable(baloorunner ${baloosearchrunner_SRCS})

target_link_libraries(baloorunner
  KF5::Runner
  KF5::KIOWidgets
  KF5::I18n
  KF5::Baloo
  KF5::Notifications
  Qt::DBus
)

install(
  TARGETS baloorunner
  DESTINATION ${KDE_INSTALL_LIBEXECDIR}
)

install(
  FILES plasma-runner-baloosearch.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/krunner/dbusplugins"
)

ecm_generate_dbus_service_file(NAME org.kde.runners.baloo
                                                           EXECUTABLE ${KDE_INSTALL_FULL_LIBEXECDIR}/baloorunner
                                                           SYSTEMD_SERVICE plasma-baloorunner.service
                                                           DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

ecm_install_configured_files(INPUT plasma-baloorunner.service.in @ONLY DESTINATION
        ${SYSTEMD_USER_UNIT_INSTALL_DIR})
