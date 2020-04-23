add_definitions(-DTRANSLATION_DOMAIN=\"libnotificationmanager\")

add_subdirectory(declarative)
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

set(notificationmanager_LIB_SRCS
    server.cpp
    server_p.cpp
    serverinfo.cpp
    settings.cpp
    mirroredscreenstracker.cpp
    notifications.cpp
    notification.cpp

    abstractnotificationsmodel.cpp
    notificationsmodel.cpp
    notificationfilterproxymodel.cpp
    notificationsortproxymodel.cpp
    notificationgroupingproxymodel.cpp
    notificationgroupcollapsingproxymodel.cpp
    watchednotificationsmodel.cpp

    jobsmodel.cpp
    jobsmodel_p.cpp
    job.cpp
    job_p.cpp

    limitedrowcountproxymodel.cpp

    utils.cpp
)

ecm_qt_declare_logging_category(notificationmanager_LIB_SRCS
    HEADER debug.h
    IDENTIFIER NOTIFICATIONMANAGER
    CATEGORY_NAME org.kde.plasma.notifications)

install(FILES libnotificationmanager.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

# Settings
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/donotdisturbsettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/notificationsettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/jobsettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/badgesettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(notificationmanager_LIB_SRCS kcfg/behaviorsettings.kcfgc GENERATE_MOC)

# DBus
# Notifications
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.freedesktop.Notifications.xml server_p.h NotificationManager::ServerPrivate)
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.kde.notificationmanager.xml server_p.h NotificationManager::ServerPrivate)
qt5_add_dbus_interface(notificationmanager_LIB_SRCS dbus/org.freedesktop.Notifications.xml fdonotifications_interface)
# JobView
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.kde.kuiserver.xml jobsmodel_p.h NotificationManager::JobsModelPrivate)
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.kde.JobViewServer.xml jobsmodel_p.h NotificationManager::JobsModelPrivate)
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.kde.JobViewServerV2.xml jobsmodel_p.h NotificationManager::JobsModelPrivate)
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.kde.JobViewV2.xml job_p.h NotificationManager::JobPrivate)
qt5_add_dbus_adaptor(notificationmanager_LIB_SRCS dbus/org.kde.JobViewV3.xml job_p.h NotificationManager::JobPrivate)

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
        KF5::I18n
        KF5::KIOFileWidgets
        KF5::Plasma
        KF5::Screen
        KF5::Service
)

set_target_properties(notificationmanager PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION 1
                      EXPORT_NAME LibNotificationManager)

install(TARGETS notificationmanager EXPORT notificationmanagerLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    server.h
    notifications.h
    notification.h
    jobsmodel.h
    job.h
    settings.h
    ${CMAKE_CURRENT_BINARY_DIR}/donotdisturbsettings.h
    ${CMAKE_CURRENT_BINARY_DIR}/notificationsettings.h
    ${CMAKE_CURRENT_BINARY_DIR}/jobsettings.h
    ${CMAKE_CURRENT_BINARY_DIR}/badgesettings.h
    ${CMAKE_CURRENT_BINARY_DIR}/behaviorsettings.h
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

install(FILES plasmanotifyrc
        DESTINATION ${KDE_INSTALL_CONFDIR})
