set(krunner_SRCS
    main.cpp
    view.cpp
)

set(krunner_dbusAppXML dbus/org.kde.krunner.App.xml)
qt5_add_dbus_adaptor(krunner_SRCS ${krunner_dbusAppXML} view.h View)
ecm_generate_dbus_service_file(NAME org.kde.krunner
                                                           EXECUTABLE ${KDE_INSTALL_FULL_BINDIR}/krunner
                                                           SYSTEMD_SERVICE plasma-krunner.service
                                                           DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

add_executable(krunner ${krunner_SRCS})

target_link_libraries(krunner
    Qt::Quick
    Qt::Widgets
    KF5::KIOGui
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

ecm_install_configured_files(INPUT plasma-krunner.service.in @ONLY DESTINATION
        ${SYSTEMD_USER_UNIT_INSTALL_DIR})

add_subdirectory(update)
