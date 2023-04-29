add_definitions(-DTRANSLATION_DOMAIN=\"kcm_notifications\")

kcmutils_add_qml_kcm(kcm_notifications)
target_sources(kcm_notifications PRIVATE
    kcm.cpp
    sourcesmodel.cpp
    filterproxymodel.cpp
    notificationsdata.cpp
)

target_link_libraries(kcm_notifications
    KF6::KCMUtils
    KF6::CoreAddons
    KF6::GlobalAccel
    KF6::GuiAddons
    KF6::I18n
    KF6::KCMUtilsQuick
    KF6::NotifyConfig
    KF6::Service
    PW::LibNotificationManager
)
