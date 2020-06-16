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
  Qt5::DBus
)

configure_file(org.kde.baloorunner.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.baloorunner.service)


install(
  TARGETS baloorunner
  DESTINATION ${KDE_INSTALL_LIBEXECDIR}
)

install(
  FILES plasma-runner-baloosearch.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/krunner/dbusplugins"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.baloorunner.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
