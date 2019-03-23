include(ECMMarkAsTest)

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_notifications\")

set(notifications_engine_SRCS
    notificationsengine.cpp
    notificationservice.cpp
    notificationaction.cpp
    notificationsanitizer.cpp
)

ecm_qt_declare_logging_category(notifications_engine_SRCS HEADER debug.h
                                               IDENTIFIER NOTIFICATIONS
                                               CATEGORY_NAME kde.dataengine.notifications`
                                               DEFAULT_SEVERITY Info)

add_library(plasma_engine_notifications MODULE ${notifications_engine_SRCS})

target_link_libraries(plasma_engine_notifications
    Qt5::DBus
    KF5::I18n
    KF5::IconThemes
    KF5::KIOCore
    KF5::Notifications
    KF5::Plasma
    KF5::Service
    KF5::NotifyConfig
    PW::LibNotificationManager
)

kcoreaddons_desktop_to_json(plasma_engine_notifications plasma-dataengine-notifications.desktop)

install(TARGETS plasma_engine_notifications DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-notifications.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install(FILES notifications.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)


#unit test
set(notifications_test_SRCS
    notificationsanitizer.cpp
    notifications_test.cpp
)

ecm_qt_declare_logging_category(notifications_test_SRCS HEADER debug.h
                                               IDENTIFIER NOTIFICATIONS
                                               CATEGORY_NAME kde.dataengine.notifications`
                                               DEFAULT_SEVERITY Info)

add_executable(notification_test  ${notifications_test_SRCS})
target_link_libraries(notification_test Qt5::Test Qt5::Core)
ecm_mark_as_test(notification_test)
