remove_definitions(-DTRANSLATION_DOMAIN=\"ksmserver\")
add_definitions(-DTRANSLATION_DOMAIN=\"kscreenlocker\")

# adjusting CMAKE_C_FLAGS to get wayland protocols to compile
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")

configure_file(config-kscreenlocker.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kscreenlocker.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(kcheckpass)
add_subdirectory(greeter)
add_subdirectory(kcm)

set(screensaver_dbusXML dbus/org.freedesktop.ScreenSaver.xml)
set(kscreensaver_dbusXML dbus/org.kde.screensaver.xml)
set(powerdevilpolicyagent_xml ${KDE4_DBUS_INTERFACES_DIR}/kf5_org.kde.Solid.PowerManagement.PolicyAgent.xml)

set(ksld_SRCS
   abstractlocker.cpp
   ksldapp.cpp
   interface.cpp
   globalaccel.cpp
   x11locker.cpp
   waylandlocker.cpp
   logind.cpp
   waylandserver.cpp
)
qt5_add_dbus_adaptor(ksld_SRCS ${screensaver_dbusXML} interface.h ScreenLocker::Interface)
qt5_add_dbus_adaptor(ksld_SRCS ${kscreensaver_dbusXML} interface.h ScreenLocker::Interface kscreensaveradaptor KScreenSaverAdaptor)
kconfig_add_kcfg_files(ksld_SRCS kcfg/kscreensaversettings.kcfgc)
qt5_add_dbus_interface(ksld_SRCS ${powerdevilpolicyagent_xml} powerdevilpolicyagent)

ecm_add_wayland_server_protocol(ksld_SRCS
    PROTOCOL protocols/ksld.xml
    BASENAME ksld
)

add_library(ksld SHARED ${ksld_SRCS})

target_link_libraries(ksld
PUBLIC
   Qt5::Core
   Qt5::X11Extras
PRIVATE
   KF5::I18n
   KF5::IconThemes
   KF5::IdleTime
   KF5::GlobalAccel
   KF5::Notifications
   KF5::XmlGui
   KF5::Crash
   KF5::WindowSystem
   ${X11_LIBRARIES}
   ${X11_Xcursor_LIB}
   XCB::XCB
   XCB::KEYSYMS
   KF5::WaylandServer
   Wayland::Server
)

if (X11_Xinput_FOUND)
    target_link_libraries(ksld PRIVATE ${X11_Xinput_LIB})
endif()

# Needed to compile on Arm target.
set_target_properties(ksld PROPERTIES COMPILE_FLAGS "-fPIC")

generate_export_header(ksld
    BASE_NAME ksld
    EXPORT_MACRO_NAME KSLD_EXPORT
)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/ScreenSaverDBusInterface")
ecm_configure_package_config_file(ScreenSaverDBusInterfaceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ScreenSaverDBusInterfaceConfig.cmake
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

set_target_properties(ksld PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS ksld  ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ScreenSaverDBusInterfaceConfig.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES kscreenlocker.notifyrc  DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} RENAME ksmserver.notifyrc)
install(FILES ${screensaver_dbusXML}
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
        RENAME kf5_org.freedesktop.ScreenSaver.xml)


install(FILES   updaters/kscreenlocker.upd DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)
install(PROGRAMS updaters/ksreenlocker_5_3_separate_autologin.pl
                DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)


add_subdirectory(autotests)
add_subdirectory(tests)
