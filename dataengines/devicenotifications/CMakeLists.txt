add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_devicenotifications\")

set(device_notifications_engine_SRCS
    devicenotificationsengine.cpp
    ksolidnotify.cpp
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

kcoreaddons_desktop_to_json(plasma_engine_devicenotifications plasma-dataengine-devicenotifications.desktop)

install(TARGETS plasma_engine_devicenotifications DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES devicenotifications.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR})
