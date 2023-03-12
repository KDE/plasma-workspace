add_definitions(-DTRANSLATION_DOMAIN=\"ksmserver\")

check_library_exists("${X11_ICE_LIB}" _IceTransNoListen "" HAVE__ICETRANSNOLISTEN)
configure_file(config-ksmserver.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksmserver.h)

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

########### next target ###############

set(ksmserver_SRCS
    main.cpp
    server.cpp
    legacy.cpp
    logout.cpp
    client.cpp
)
ecm_qt_declare_logging_category(ksmserver_SRCS HEADER ksmserver_debug.h IDENTIFIER KSMSERVER CATEGORY_NAME org.kde.kf6.ksmserver)

qt_add_dbus_adaptor( ksmserver_SRCS org.kde.KSMServerInterface.xml server.h KSMServer )
qt_add_dbus_interface( ksmserver_SRCS ${KSCREENLOCKER_DBUS_INTERFACES_DIR}/org.kde.screensaver.xml kscreenlocker_interface )
qt_add_dbus_interface( ksmserver_SRCS org.kde.LogoutPrompt.xml logoutprompt_interface)
qt_add_dbus_interface( ksmserver_SRCS org.kde.KWin.Session.xml kwinsession_interface)

qt_add_dbus_interface( ksmserver_SRCS ../startkde/plasma-session/org.kde.Startup.xml startup_interface )
qt_add_dbus_interface( ksmserver_SRCS ../startkde/plasma-shutdown/org.kde.Shutdown.xml shutdown_interface )

add_executable(ksmserver ${ksmserver_SRCS})

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KSMServerDBusInterface")

configure_package_config_file(KSMServerDBusInterfaceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KSMServerDBusInterfaceConfig.cmake
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

target_link_libraries(ksmserver
    PW::KScreenLocker
    PW::KWorkspace
    KF6::XmlGui
    KF6::GlobalAccel
    KF6::I18n
    KF6::KIOCore
    KF6::KIOGui
    KF6::Solid
    KF6::DBusAddons
    KF6::Notifications
    KF6::Package
    KF6::WindowSystem
    Qt::Concurrent
    X11::X11
    X11::SM
    X11::ICE
)
if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(ksmserver Qt::X11Extras)
else()
    target_link_libraries(ksmserver Qt::GuiPrivate)
endif()

install(TARGETS ksmserver ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KSMServerDBusInterfaceConfig.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})


########### install files ###############

install( FILES org.kde.KSMServerInterface.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

ecm_install_configured_files(INPUT plasma-ksmserver.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
ecm_install_configured_files(INPUT plasma-restoresession.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
