add_definitions(-DTRANSLATION_DOMAIN=\"kded_devicenotifications\")

kcoreaddons_add_plugin(devicenotifications SOURCES devicenotifications.cpp INSTALL_NAMESPACE "kf6/kded")
target_link_libraries(devicenotifications
    KF6::DBusAddons
    KF6::CoreAddons
    KF6::I18n
    KF6::Notifications
    UDev::UDev
)
