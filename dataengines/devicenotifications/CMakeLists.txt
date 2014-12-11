
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
) # todo: add kworkspace once ported

kcoreaddons_desktop_to_json(plasma_engine_devicenotifications plasma-dataengine-devicenotifications.desktop)

install(TARGETS plasma_engine_devicenotifications DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-devicenotifications.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
