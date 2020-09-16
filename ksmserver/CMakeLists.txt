add_definitions(-DTRANSLATION_DOMAIN=\"ksmserver\")

check_library_exists("${X11_ICE_LIB}" _IceTransNoListen "" HAVE__ICETRANSNOLISTEN)
configure_file(config-ksmserver.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksmserver.h)

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

########### next target ###############

set(ksmserver_SRCS
    main.cpp
    server.cpp
    legacy.cpp
    logout.cpp
    client.cpp
)
ecm_qt_declare_logging_category(ksmserver_SRCS HEADER ksmserver_debug.h IDENTIFIER KSMSERVER CATEGORY_NAME org.kde.kf5.ksmserver)

qt5_add_dbus_adaptor( ksmserver_SRCS org.kde.KSMServerInterface.xml server.h KSMServer )
qt5_add_dbus_interface( ksmserver_SRCS ${KSCREENLOCKER_DBUS_INTERFACES_DIR}/org.kde.screensaver.xml kscreenlocker_interface )
qt5_add_dbus_interface( ksmserver_SRCS org.kde.LogoutPrompt.xml logoutprompt_interface)
qt5_add_dbus_interface( ksmserver_SRCS org.kde.KWin.Session.xml kwinsession_interface)


set(klauncher_xml ${KINIT_DBUS_INTERFACES_DIR}/kf5_org.kde.KLauncher.xml)
qt5_add_dbus_interface( ksmserver_SRCS ${klauncher_xml} klauncher_interface )

qt5_add_dbus_interface( ksmserver_SRCS ../startkde/plasma-session/org.kde.Startup.xml startup_interface )
qt5_add_dbus_interface( ksmserver_SRCS ../startkde/plasma-shutdown/org.kde.Shutdown.xml shutdown_interface )

add_executable(ksmserver ${ksmserver_SRCS})

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KSMServerDBusInterface")

configure_package_config_file(KSMServerDBusInterfaceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KSMServerDBusInterfaceConfig.cmake
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

target_link_libraries(ksmserver
    PW::KScreenLocker
    PW::KWorkspace
    KF5::XmlGui
    KF5::GlobalAccel
    KF5::I18n
    KF5::KIOCore
    KF5::KIOGui
    ${X11_LIBRARIES}
    ${X11_Xrender_LIB}
    Qt5::X11Extras
    KF5::Solid
    Qt5::Quick
    KF5::Declarative
    KF5::DBusAddons
    KF5::Notifications
    KF5::Package
    KF5::WindowSystem
    Qt5::Concurrent
)

install(TARGETS ksmserver ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KSMServerDBusInterfaceConfig.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})


########### install files ###############

install( FILES org.kde.KSMServerInterface.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

ecm_install_configured_files(INPUT plasma-ksmserver.service.in DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})
