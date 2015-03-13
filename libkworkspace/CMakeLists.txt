
set(kworkspace_LIB_SRCS kdisplaymanager.cpp
                        kworkspace.cpp
   )

add_definitions(-DTRANSLATION_DOMAIN=\"libkworkspace\")
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
remove_definitions(-DQT_NO_CAST_TO_ASCII)


set(ksmserver_xml  ${plasma-workspace_SOURCE_DIR}/ksmserver/org.kde.KSMServerInterface.xml)
qt5_add_dbus_interface( kworkspace_LIB_SRCS ${ksmserver_xml} ksmserver_interface )

set_source_files_properties(${KWIN_INTERFACE} PROPERTIES INCLUDE "interface_util.h")

qt5_add_dbus_interface( kworkspace_LIB_SRCS ${KWIN_INTERFACE} kwin_interface )


add_library(kworkspace ${kworkspace_LIB_SRCS})
add_library(PW::KWorkspace ALIAS kworkspace)
set_target_properties(kworkspace PROPERTIES
                             VERSION ${PROJECT_VERSION}
                             SOVERSION ${PROJECT_VERSION_MAJOR}
                             EXPORT_NAME KWorkspace
                             OUTPUT_NAME kworkspace5
                      )

generate_export_header(kworkspace)
target_link_libraries(kworkspace
    PUBLIC
        Qt5::Core
    PRIVATE
        Qt5::DBus
        KF5::I18n
        KF5::WindowSystem
        KF5::Plasma
)
target_include_directories(kworkspace PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                                      INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/kworkspace5>" )

configure_file(config-libkworkspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-libkworkspace.h)

if(X11_FOUND)
    if(NOT X11_Xau_FOUND)
        message(FATAL_ERROR "Found X11, but not libXau which is required for building kworkspace")
    endif()
    target_link_libraries(kworkspace PRIVATE Qt5::X11Extras ${X11_LIBRARIES} ${X11_Xau_LIB})
endif()

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfigVersion.cmake VERSION
                                     "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

install(TARGETS kworkspace EXPORT libkworkspaceLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kdisplaymanager.h
               kworkspace.h
               ${CMAKE_CURRENT_BINARY_DIR}/config-libkworkspace.h
               ${CMAKE_CURRENT_BINARY_DIR}/kworkspace_export.h
         DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kworkspace5 COMPONENT Devel )

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_LIBDIR}/cmake/LibKWorkspace)
ecm_configure_package_config_file(LibKWorkspaceConfig.cmake.in
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
