add_subdirectory(declarative)
if(BUILD_TESTING)
   #add_subdirectory(autotests)
endif()

set(notificationmanager_LIB_SRCS
    notificationserver.cpp
    notificationserver_p.cpp
    notification.cpp
    notificationmodel.cpp

    jobsmodel.cpp
    job.cpp
    jobdetails.cpp

    notifications.cpp
    notificationfilterproxymodel.cpp
    notificationsortproxymodel.cpp
    notificationgroupingproxymodel.cpp
    limitedrowcountproxymodel.cpp

    settings.cpp
)

ecm_qt_declare_logging_category(notificationmanager_LIB_SRCS
    HEADER debug.h
    IDENTIFIER NOTIFICATIONMANAGER
    CATEGORY_NAME org.kde.plasma.notifications)
install(FILES libnotificationmanager.categories DESTINATION ${KDE_INSTALL_CONFDIR})

# Settings
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/donotdisturbsettings.kcfgc)
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/notificationsettings.kcfgc)
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/jobsettings.kcfgc)
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/badgesettings.kcfgc)

# DBus
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.freedesktop.Notifications.xml notificationserver_p.h NotificationManager::NotificationServerPrivate)

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
        KF5::ConfigCore
        KF5::ItemModels
    PRIVATE
        Qt5::DBus
        KF5::ConfigGui
        KF5::Service
        KF5::Plasma
        KF5::I18n
        KF5::IconThemes
        KF5::ProcessCore
)

set_target_properties(notificationmanager PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION 1
                      EXPORT_NAME LibNotificationManager)

install(TARGETS notificationmanager EXPORT notificationmanagerLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    notificationserver.h
    notification.h
    settings.h
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
