add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_devicenotifications\")

set(device_notifications_engine_SRCS
    devicenotificationsengine.cpp
    ksolidnotify.cpp
    devicenotificationsengine.h
    ksolidnotify.h
)

add_library(plasma_engine_devicenotifications MODULE ${device_notifications_engine_SRCS})

target_link_libraries(plasma_engine_devicenotifications 
    KF6::Service
    Plasma::Plasma
    KF6::Plasma5Support
    KF6::Solid
    KF6::I18n
    KSysGuard::ProcessCore
    KF6::Notifications
) # todo: add kworkspace once ported

install(TARGETS plasma_engine_devicenotifications DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)
install(FILES devicenotifications.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
