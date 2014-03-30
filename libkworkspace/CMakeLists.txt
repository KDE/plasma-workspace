
set(kworkspace_LIB_SRCS kdisplaymanager.cpp
                        kworkspace.cpp
   )

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
                             VERSION 4.95.0
                             SOVERSION 5
                             EXPORT_NAME KWorkspace
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
        Xau
)
target_include_directories(kworkspace PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                                      INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/kworkspace>" )

configure_file(config-libkworkspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-libkworkspace.h)

if(X11_FOUND)
    target_link_libraries(kworkspace PRIVATE Qt5::X11Extras ${X11_LIBRARIES})
endif()

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfigVersion.cmake VERSION
                                     "${PLASMAWORKSPACE_VERSION_STRING}" COMPATIBILITY AnyNewerVersion)

install(TARGETS kworkspace EXPORT libkworkspaceLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kdisplaymanager.h
               kworkspace.h
               ${CMAKE_CURRENT_BINARY_DIR}/config-libkworkspace.h
               ${CMAKE_CURRENT_BINARY_DIR}/kworkspace_export.h
         DESTINATION ${INCLUDE_INSTALL_DIR}/kworkspace COMPONENT Devel )

set(CMAKECONFIG_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/LibKWorkspace)
ecm_configure_package_config_file(LibKWorkspaceConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibKWorkspaceConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libkworkspaceLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibKWorkspaceLibraryTargets.cmake )
