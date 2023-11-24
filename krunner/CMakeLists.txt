set(krunner_SRCS
    main.cpp
    view.cpp
    x11windowscreenrelativepositioner.cpp
    qml/krunner.qrc
)

set(krunner_dbusAppXML dbus/org.kde.krunner.App.xml)
qt_add_dbus_adaptor(krunner_SRCS ${krunner_dbusAppXML} view.h View)
ecm_generate_dbus_service_file(
    NAME org.kde.krunner
    EXECUTABLE "${KDE_INSTALL_FULL_BINDIR}/krunner --daemon"
    SYSTEMD_SERVICE plasma-krunner.service
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

add_executable(krunner ${krunner_SRCS})

target_link_libraries(krunner
    Qt::Quick
    Qt::Widgets
    KF6::KIOGui
    KF6::I18n
    Plasma::PlasmaQuick
    Plasma::Activities
    KF6::DBusAddons
    KF6::Crash
    KF6::Runner
    PW::KWorkspace
    LayerShellQt::Interface
)
target_compile_definitions(krunner PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

install(TARGETS krunner ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${krunner_dbusAppXML} DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

configure_file(org.kde.krunner.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.desktop @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.desktop DESTINATION ${KDE_INSTALL_DATADIR}/kglobalaccel)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KRunnerAppDBusInterface")
configure_package_config_file(KRunnerAppDBusInterfaceConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake"
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})

ecm_install_configured_files(
    INPUT plasma-krunner.service.in
    @ONLY
    DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)

