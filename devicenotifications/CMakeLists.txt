add_definitions(-DTRANSLATION_DOMAIN=\"kded_devicenotifications\")

kcoreaddons_add_plugin(devicenotifications SOURCES devicenotifications.cpp devicenotifications.h INSTALL_NAMESPACE "kf6/kded")

qt6_generate_wayland_protocol_client_sources(devicenotifications
    FILES ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-device-v2.xml PRIVATE_CODE
)

target_link_libraries(devicenotifications
    KF6::DBusAddons
    KF6::CoreAddons
    KF6::I18n
    KF6::Notifications
    UDev::UDev
    Wayland::Client
)
