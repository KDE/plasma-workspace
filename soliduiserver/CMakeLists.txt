########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"soliduiserver5\")

kcoreaddons_add_plugin(soliduiserver SOURCES soliduiserver.cpp INSTALL_NAMESPACE "kf5/kded")
kcoreaddons_desktop_to_json(soliduiserver soliduiserver.desktop)
target_link_libraries(soliduiserver
    KF5::Solid
    KF5::DBusAddons
    KF5::Wallet
    KF5::KIOCore
    KF5::WindowSystem
    KF5::I18n
    KF5::WidgetsAddons
)
