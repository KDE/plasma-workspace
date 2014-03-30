
set(device_notifications_engine_SRCS
    devicenotificationsengine.cpp
)

qt5_add_dbus_adaptor( device_notifications_engine_SRCS org.kde.DeviceNotifications.xml devicenotificationsengine.h DeviceNotificationsEngine )

add_library(plasma_engine_devicenotifications MODULE ${device_notifications_engine_SRCS})

target_link_libraries(plasma_engine_devicenotifications 
    Qt5::DBus
    KF5::Service
    KF5::Plasma
) # todo: add kworkspace once ported

kservice_desktop_to_json(plasma_engine_devicenotifications plasma-dataengine-devicenotifications.desktop)

install(TARGETS plasma_engine_devicenotifications DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-devicenotifications.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
