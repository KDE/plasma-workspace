add_definitions(-DTRANSLATION_DOMAIN=\"kcm_notifications\")

kcmutils_add_qml_kcm(kcm_notifications)
target_sources(kcm_notifications PRIVATE
    kcm.cpp
    sourcesmodel.cpp
    filterproxymodel.cpp
    notificationsdata.cpp
    soundthemeconfig.cpp
)

ecm_qt_declare_logging_category(kcm_notifications
    HEADER kcm_notifications_debug.h
    IDENTIFIER KCM_NOTIFICATIONS
    CATEGORY_NAME org.kde.kcm_notifications
    EXPORT KCM_NOTIFICATIONS
    DESCRIPTION "System Settings - Notifications"
)

target_link_libraries(kcm_notifications PRIVATE
    KF6::KCMUtils
    KF6::CoreAddons
    KF6::GlobalAccel
    KF6::GuiAddons
    KF6::I18n
    KF6::KCMUtilsQuick
    KF6::Service
    PW::LibNotificationManager
    Canberra::Canberra
)
