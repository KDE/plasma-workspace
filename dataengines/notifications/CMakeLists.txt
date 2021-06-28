add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_notifications\")

set(notifications_engine_SRCS
    notificationsengine.cpp
    notificationservice.cpp
    notificationaction.cpp
)

ecm_qt_declare_logging_category(notifications_engine_SRCS HEADER debug.h
                                               IDENTIFIER NOTIFICATIONS
                                               CATEGORY_NAME kde.dataengine.notifications`
                                               DEFAULT_SEVERITY Info)

kcoreaddons_add_plugin(plasma_engine_notifications SOURCES ${notifications_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

target_link_libraries(plasma_engine_notifications
    Qt::DBus
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

install(FILES notifications.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
