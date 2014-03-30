add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1223)
add_subdirectory(kcheckpass)
add_subdirectory(data)
add_subdirectory(greeter)

set(screensaver_dbusXML dbus/org.freedesktop.ScreenSaver.xml)
set(kscreensaver_dbusXML dbus/org.kde.screensaver.xml)
set(ksmserver_xml  ${ksmserver_SOURCE_DIR}/org.kde.KSMServerInterface.xml)
set(powerdevilpolicyagent_xml ${SOLID_DBUS_INTERFACES_DIR}/kf5_org.kde.Solid.PowerManagement.PolicyAgent.xml)

set(ksld_SRCS
   ksldapp.cpp
   autologout.cpp
   interface.cpp
   lockwindow.cpp)
qt5_add_dbus_adaptor(ksld_SRCS ${screensaver_dbusXML} interface.h ScreenLocker::Interface)
qt5_add_dbus_adaptor(ksld_SRCS ${kscreensaver_dbusXML} interface.h ScreenLocker::Interface kscreensaveradaptor KScreenSaverAdaptor)
kconfig_add_kcfg_files(ksld_SRCS kcfg/kscreensaversettings.kcfgc)
qt5_add_dbus_interface(ksld_SRCS ${ksmserver_xml} ksmserver_interface)
qt5_add_dbus_interface(ksld_SRCS ${powerdevilpolicyagent_xml} powerdevilpolicyagent)

add_library(screenlocker_static STATIC ${ksld_SRCS})

target_link_libraries(screenlocker_static
   PW::KWorkspace
   Qt5::X11Extras
   KF5::IdleTime
   KF5::GlobalAccel
   KF5::XmlGui
   KF5::Crash
   KF5::KDE4Support
   ${X11_LIBRARIES}
   ${X11_Xcursor_LIB}
   ${XCB_XCB_LIBRARY}
)

# Needed to compile on Arm target.
set_target_properties(screenlocker_static PROPERTIES COMPILE_FLAGS "-fPIC")

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/ScreenSaverDBusInterface")
ecm_configure_package_config_file(ScreenSaverDBusInterfaceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ScreenSaverDBusInterfaceConfig.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES kscreenlocker.notifyrc  DESTINATION ${DATA_INSTALL_DIR}/ksmserver/ RENAME ksmserver.notifyrc)
install(FILES ${screensaver_dbusXML}
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
        RENAME kf5_org.freedesktop.ScreenSaver.xml)
