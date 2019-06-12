########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"soliduiserver5\")

set(kded_soliduiserver_SRCS
   soliduiserver.cpp
)

add_library(soliduiserver MODULE ${kded_soliduiserver_SRCS})
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

install(TARGETS soliduiserver DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
