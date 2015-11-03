remove_definitions(-DTRANSLATION_DOMAIN=\"ksmserver\")
add_definitions(-DTRANSLATION_DOMAIN=\"kscreenlocker\")

# adjusting CMAKE_C_FLAGS to get wayland protocols to compile
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")

include(ECMSetupVersion)
include(ECMGenerateHeaders)

ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX KSCREENLOCKER
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kscreenlocker_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KScreenLockerConfigVersion.cmake"
                        SOVERSION 5)

configure_file(config-kscreenlocker.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kscreenlocker.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(KSLD_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/KScreenLocker")

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

add_library(KScreenLocker SHARED ${ksld_SRCS})

target_link_libraries(KScreenLocker
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
    target_link_libraries(KScreenLocker PRIVATE ${X11_Xinput_LIB})
endif()

target_include_directories(KScreenLocker INTERFACE "$<INSTALL_INTERFACE:${KSLD_INCLUDEDIR}>")

# Needed to compile on Arm target.
set_target_properties(KScreenLocker PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(PW::KScreenLocker ALIAS KScreenLocker)

generate_export_header(KScreenLocker
    BASE_NAME KScreenLocker
    EXPORT_MACRO_NAME KSCREENLOCKER_EXPORT
    EXPORT_FILE_NAME KScreenLocker/kscreenlocker_export.h
)

ecm_configure_package_config_file(ScreenSaverDBusInterfaceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ScreenSaverDBusInterfaceConfig.cmake
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_PREFIX}/ScreenSaverDBusInterface")

set_target_properties(KScreenLocker PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ScreenSaverDBusInterfaceConfig.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_PREFIX}/ScreenSaverDBusInterface")

ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/KScreenLockerConfig.cmake.in"
								  "${CMAKE_CURRENT_BINARY_DIR}/KScreenLockerConfig.cmake"
								  INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_PREFIX}/KScreenLocker")

install(TARGETS KScreenLocker EXPORT KScreenLockerTargets ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY)
install(EXPORT KScreenLockerTargets DESTINATION "${CMAKECONFIG_INSTALL_PREFIX}/KScreenLocker" FILE KScreenLockerTargets.cmake NAMESPACE PW::)

ecm_generate_headers(KScreenLocker_CamelCase_HEADERS
    HEADER_NAMES
        KsldApp
    REQUIRED_HEADERS KScreenLocker_HEADERS)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/KScreenLocker/kscreenlocker_export.h
        ${KScreenLocker_CamelCase_HEADERS}
        ${KScreenLocker_HEADERS}
    DESTINATION
        ${KSLD_INCLUDEDIR}/KScreenLocker
    COMPONENT Devel)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/KScreenLockerConfig.cmake"
			  "${CMAKE_CURRENT_BINARY_DIR}/KScreenLockerConfigVersion.cmake"
		DESTINATION "${CMAKECONFIG_INSTALL_PREFIX}/KScreenLocker"
		COMPONENT Devel)

install(FILES kscreenlocker.notifyrc  DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} RENAME ksmserver.notifyrc)
install(FILES ${screensaver_dbusXML}
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
        RENAME kf5_org.freedesktop.ScreenSaver.xml)


install(FILES   updaters/kscreenlocker.upd DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)
install(PROGRAMS updaters/ksreenlocker_5_3_separate_autologin.pl
                DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)

add_subdirectory(autotests)
add_subdirectory(tests)
