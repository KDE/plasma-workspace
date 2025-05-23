
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-outputorder.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-outputorder.h)

set(kworkspace_LIB_SRCS kdisplaymanager.cpp
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
    defaultservice.h
    defaultwallpaper.cpp
    defaultwallpaper.h
    outputorderwatcher.cpp
    outputorderwatcher.h
   )

add_definitions(-DTRANSLATION_DOMAIN=\"libkworkspace\")

qt_add_dbus_interface(kworkspace_LIB_SRCS ${SCREENSAVER_DBUS_INTERFACE} screenlocker_interface )
qt_add_dbus_interface(kworkspace_LIB_SRCS ${plasma-workspace_SOURCE_DIR}/ksmserver/org.kde.LogoutPrompt.xml logoutprompt_interface)
qt_add_dbus_interface(kworkspace_LIB_SRCS ${plasma-workspace_SOURCE_DIR}/startkde/plasma-session/org.kde.Startup.xml startup_interface)
qt_add_dbus_interface(kworkspace_LIB_SRCS ${plasma-workspace_SOURCE_DIR}/startkde/plasma-shutdown/org.kde.Shutdown.xml shutdown_interface)

add_library(kworkspace ${kworkspace_LIB_SRCS})
add_library(PW::KWorkspace ALIAS kworkspace)
set_target_properties(kworkspace PROPERTIES
                             VERSION ${PROJECT_VERSION}
                             SOVERSION ${PROJECT_VERSION_MAJOR}
                             EXPORT_NAME KWorkspace
                             OUTPUT_NAME kworkspace6
                      )

qt_extract_metatypes(kworkspace)

generate_export_header(kworkspace)
target_link_libraries(kworkspace
    PUBLIC
        Qt::Core
        KF6::CoreAddons
    PRIVATE
        Qt::DBus
        Qt::WaylandClient
        KF6::I18n
        KF6::WindowSystem
        KF6::ConfigCore
        KF6::Service
        KF6::Package
        Plasma::Plasma
)
target_include_directories(kworkspace PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                                      INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/kworkspace6>" )

configure_file(config-libkworkspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-libkworkspace.h)

if(HAVE_X11)
    target_link_libraries(kworkspace PRIVATE X11::Xau XCB::XCB XCB::RANDR)
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

if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
    set(private_code_option  "PRIVATE_CODE")
endif()
qt6_generate_wayland_protocol_client_sources(kworkspace
    ${private_code_option}
    FILES
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
)

install(TARGETS kworkspace EXPORT libkworkspaceLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kdisplaymanager.h
               kworkspace.h
               sessionmanagement.h
               autostartscriptdesktopfile.h
               outputorderwatcher.h
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
