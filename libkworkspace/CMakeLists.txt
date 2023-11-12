
set(kworkspace_LIB_SRCS kdisplaymanager.cpp
    kworkspace.cpp
    sessionmanagement.cpp
    sessionmanagementbackend.cpp
    autostartscriptdesktopfile.cpp
    kdisplaymanager.h
    kworkspace.h
    sessionmanagement.h
    sessionmanagementbackend.h
    autostartscriptdesktopfile.h
    login1_manager_interface.cpp
    login1_manager_interface.h
    defaultservice.cpp
   )

add_definitions(-DTRANSLATION_DOMAIN=\"libkworkspace\")

qt_add_dbus_interface(kworkspace_LIB_SRCS kf6_org.kde.KLauncher.xml klauncher_interface)
qt_add_dbus_interface(kworkspace_LIB_SRCS ${KSCREENLOCKER_DBUS_INTERFACES_DIR}/kf6_org.freedesktop.ScreenSaver.xml screenlocker_interface )
qt_add_dbus_interface(kworkspace_LIB_SRCS ${KSCREENLOCKER_DBUS_INTERFACES_DIR}/org.kde.screensaver.xml kscreenlocker_interface )
qt_add_dbus_interface(kworkspace_LIB_SRCS ${plasma-workspace_SOURCE_DIR}/ksmserver/org.kde.LogoutPrompt.xml logoutprompt_interface)
qt_add_dbus_interface(kworkspace_LIB_SRCS ${plasma-workspace_SOURCE_DIR}/startkde/plasma-session/org.kde.Startup.xml startup_interface)
qt_add_dbus_interface(kworkspace_LIB_SRCS ${plasma-workspace_SOURCE_DIR}/startkde/plasma-shutdown/org.kde.Shutdown.xml shutdown_interface)

set(ksmserver_xml ${plasma-workspace_SOURCE_DIR}/ksmserver/org.kde.KSMServerInterface.xml)
qt_add_dbus_interface( kworkspace_LIB_SRCS ${ksmserver_xml} ksmserver_interface )

add_library(kworkspace ${kworkspace_LIB_SRCS})
add_library(PW::KWorkspace ALIAS kworkspace)
set_target_properties(kworkspace PROPERTIES
                             VERSION ${PROJECT_VERSION}
                             SOVERSION ${PROJECT_VERSION_MAJOR}
                             EXPORT_NAME KWorkspace
                             OUTPUT_NAME kworkspace6
                      )

generate_export_header(kworkspace)
target_link_libraries(kworkspace
    PUBLIC
        Qt::Core
        KF6::CoreAddons
    PRIVATE
        Qt::DBus
        KF6::I18n
        KF6::WindowSystem
        KF6::ConfigCore
        KF6::Service
)
target_include_directories(kworkspace PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                                      INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/kworkspace6>" )

configure_file(config-libkworkspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-libkworkspace.h)

if(HAVE_X11)
    target_link_libraries(kworkspace PRIVATE X11::Xau)
endif()

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfigVersion.cmake VERSION
                                     "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)


ecm_qt_declare_logging_category(kworkspace
    HEADER libkworkspace_debug.h
    IDENTIFIER LIBKWORKSPACE_DEBUG
    CATEGORY_NAME org.kde.plasma.libkworkspace
    DESCRIPTION "lib kworkspace debug"
    EXPORT PLASMAWORKSPACE
)

install(TARGETS kworkspace EXPORT libkworkspaceLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kdisplaymanager.h
               kworkspace.h
               sessionmanagement.h
               autostartscriptdesktopfile.h
               ${CMAKE_CURRENT_BINARY_DIR}/config-libkworkspace.h
               ${CMAKE_CURRENT_BINARY_DIR}/kworkspace_export.h
         DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kworkspace6 COMPONENT Devel )

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/LibKWorkspace)
configure_package_config_file(LibKWorkspaceConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libkworkspaceLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibKWorkspaceLibraryTargets.cmake )

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
