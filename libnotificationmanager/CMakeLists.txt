add_subdirectory(declarative)
if(BUILD_TESTING)
   #add_subdirectory(autotests)
endif()

set(notificationmanager_LIB_SRCS
    notificationserver.cpp
    notification.cpp
    notificationmodel.cpp
)

qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS org.freedesktop.Notifications.xml notificationserver.h  NotificationManager::NotificationServer)

add_library(notificationmanager ${notificationmanager_LIB_SRCS})
add_library(PW::LibNotificationManager ALIAS notificationmanager)

target_compile_definitions(notificationmanager PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

generate_export_header(notificationmanager)

target_include_directories(notificationmanager PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/notificationmanager>")

target_link_libraries(notificationmanager
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Quick
        #KF5::ItemModels
    PRIVATE
        Qt5::DBus
        KF5::ConfigCore
        KF5::Service
)

set_target_properties(notificationmanager PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION 1
                      EXPORT_NAME LibNotificationManager)

install(TARGETS notificationmanager EXPORT notificationmanagerLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    #notificationserver.h # FIXME dont export this class
    ${CMAKE_CURRENT_BINARY_DIR}/notificationmanager_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/notificationmanager COMPONENT Devel
)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibNotificationManagerConfigVersion.cmake VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_LIBDIR}/cmake/LibNotificationManager)

configure_package_config_file(LibNotificationManagerConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibNotificationManagerConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibNotificationManagerConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/LibNotificationManagerConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT notificationmanagerLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibNotificationManagerLibraryTargets.cmake )
