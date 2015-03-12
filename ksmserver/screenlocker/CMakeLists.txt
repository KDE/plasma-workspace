remove_definitions(-DTRANSLATION_DOMAIN=\"ksmserver\")
add_definitions(-DTRANSLATION_DOMAIN=\"kscreenlocker\")

# adjusting CMAKE_C_FLAGS to get wayland protocols to compile
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1223)
add_subdirectory(kcheckpass)
add_subdirectory(greeter)
add_subdirectory(kcm)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

set(screensaver_dbusXML dbus/org.freedesktop.ScreenSaver.xml)
set(kscreensaver_dbusXML dbus/org.kde.screensaver.xml)
set(ksmserver_xml  ${PROJECT_SOURCE_DIR}/ksmserver/org.kde.KSMServerInterface.xml)
set(powerdevilpolicyagent_xml ${KDE4_DBUS_INTERFACES_DIR}/kf5_org.kde.Solid.PowerManagement.PolicyAgent.xml)

set(ksld_SRCS
   ksldapp.cpp
   interface.cpp
   globalaccel.cpp
   lockwindow.cpp
   logind.cpp
   waylandserver.cpp
)
qt5_add_dbus_adaptor(ksld_SRCS ${screensaver_dbusXML} interface.h ScreenLocker::Interface)
qt5_add_dbus_adaptor(ksld_SRCS ${kscreensaver_dbusXML} interface.h ScreenLocker::Interface kscreensaveradaptor KScreenSaverAdaptor)
kconfig_add_kcfg_files(ksld_SRCS kcfg/kscreensaversettings.kcfgc)
qt5_add_dbus_interface(ksld_SRCS ${ksmserver_xml} ksmserver_interface)
qt5_add_dbus_interface(ksld_SRCS ${powerdevilpolicyagent_xml} powerdevilpolicyagent)

ecm_add_wayland_server_protocol(ksld_SRCS
    PROTOCOL protocols/ksld.xml
    BASENAME ksld
)

add_library(screenlocker_static STATIC ${ksld_SRCS})

target_link_libraries(screenlocker_static
   PW::KWorkspace
   Qt5::X11Extras
   KF5::I18n
   KF5::IconThemes
   KF5::IdleTime
   KF5::I18n
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
    target_link_libraries(screenlocker_static ${X11_Xinput_LIB})
endif()

# Needed to compile on Arm target.
set_target_properties(screenlocker_static PROPERTIES COMPILE_FLAGS "-fPIC")

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/ScreenSaverDBusInterface")
ecm_configure_package_config_file(ScreenSaverDBusInterfaceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ScreenSaverDBusInterfaceConfig.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ScreenSaverDBusInterfaceConfig.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES kscreenlocker.notifyrc  DESTINATION ${KNOTIFYRC_INSTALL_DIR} RENAME ksmserver.notifyrc)
install(FILES ${screensaver_dbusXML}
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
        RENAME kf5_org.freedesktop.ScreenSaver.xml)


install(FILES   updaters/kscreenlocker.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)
install(PROGRAMS updaters/ksreenlocker_5_3_separate_autologin.pl
                DESTINATION ${DATA_INSTALL_DIR}/kconf_update)


add_subdirectory(autotests)
add_subdirectory(tests)
