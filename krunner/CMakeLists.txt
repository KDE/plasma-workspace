set(krunner_SRCS
    #    fullview.cpp
    main.cpp
    view.cpp
)

find_package(KF5Declarative ${KF5_VERSION} REQUIRED)
find_package(KF5Plasma ${KF5_VERSION} REQUIRED)
find_package(KF5PlasmaQuick REQUIRED NO_MODULE)
find_package(KF5Config REQUIRED NO_MODULE)

set(krunner_dbusAppXML dbus/org.kde.krunner.App.xml)
qt5_add_dbus_adaptor(krunner_SRCS ${krunner_dbusAppXML} view.h View)
configure_file(dbus/org.kde.krunner.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})


add_executable(krunner ${krunner_SRCS})

target_link_libraries(krunner
    Qt5::Widgets
    Qt5::Quick
    KF5::Declarative
    KF5::I18n
    KF5::Plasma
    KF5::PlasmaQuick
    KF5::GlobalAccel
    KF5::DBusAddons
    KF5::WindowSystem
    KF5::ConfigWidgets
    KF5::Crash
)

install(TARGETS krunner ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${krunner_dbusAppXML} DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KRunnerAppDBusInterface")
ecm_configure_package_config_file(KRunnerAppDBusInterfaceConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})
