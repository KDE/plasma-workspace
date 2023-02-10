# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_notifications\")

set(kcm_notifications_SRCS
    kcm.cpp
    sourcesmodel.cpp
    filterproxymodel.cpp
    notificationsdata.cpp
)

kcoreaddons_add_plugin(kcm_notifications SOURCES ${kcm_notifications_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file(kcm_notifications)
target_link_libraries(kcm_notifications
    KF6::KCMUtils
    KF6::CoreAddons
    KF6::Declarative
    KF6::GlobalAccel
    KF6::GuiAddons
    KF6::I18n
    KF6::QuickAddons
    KF6::NotifyConfig
    KF6::Service
    PW::LibNotificationManager
)

kpackage_install_package(package kcm_notifications kcms)

