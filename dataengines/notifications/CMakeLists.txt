add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_notifications\")

set(notifications_engine_SRCS
    notificationsengine.cpp
    notificationservice.cpp
    notificationaction.cpp
    notificationsengine.h
    notificationservice.h
    notificationaction.h
)

ecm_qt_declare_logging_category(notifications_engine_SRCS HEADER debug.h
                                               IDENTIFIER NOTIFICATIONS
                                               CATEGORY_NAME kde.dataengine.notifications`
                                               DEFAULT_SEVERITY Info
                                               DESCRIPTION "DataEngine notifications"
                                               EXPORT PLASMAWORKSPACE
                                           )

kcoreaddons_add_plugin(plasma_engine_notifications SOURCES ${notifications_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_notifications
    Qt::DBus
    KF6::I18n
    KF6::IconThemes
    KF6::KIOCore
    KF6::Notifications
    Plasma::Plasma
    Plasma::Plasma5Support
    KF6::Service
    KF6::NotifyConfig
    PW::LibNotificationManager
)

install(FILES notifications.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
