set(krunner_SRCS
    main.cpp
    view.cpp
)

set(krunner_dbusAppXML dbus/org.kde.krunner.App.xml)
qt5_add_dbus_adaptor(krunner_SRCS ${krunner_dbusAppXML} view.h View)
configure_file(dbus/org.kde.krunner.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})


add_executable(krunner ${krunner_SRCS})

target_link_libraries(krunner
    Qt5::Quick
    Qt5::Widgets
    KF5::KIOWidgets
    KF5::Declarative
    KF5::I18n
    KF5::PlasmaQuick
    KF5::DBusAddons
    KF5::Crash
    KF5::WaylandClient
    KF5::QuickAddons
    PW::KWorkspace
)
target_compile_definitions(krunner PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

install(TARGETS krunner ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${krunner_dbusAppXML} DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

install(FILES krunner.desktop DESTINATION ${DATA_INSTALL_DIR}/kglobalaccel)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KRunnerAppDBusInterface")
configure_package_config_file(KRunnerAppDBusInterfaceConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake"
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})

add_subdirectory(update)
