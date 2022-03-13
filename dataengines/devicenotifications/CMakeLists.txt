add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_devicenotifications\")

set(device_notifications_engine_SRCS
    devicenotificationsengine.cpp
    ksolidnotify.cpp
    devicenotificationsengine.h
    ksolidnotify.h
)

add_library(plasma_engine_devicenotifications MODULE ${device_notifications_engine_SRCS})

target_link_libraries(plasma_engine_devicenotifications 
    KF5::Service
    KF5::Plasma
    KF5::Solid
    KF5::I18n
    KSysGuard::ProcessCore
    KF5::Notifications
) # todo: add kworkspace once ported

install(TARGETS plasma_engine_devicenotifications DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES devicenotifications.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
