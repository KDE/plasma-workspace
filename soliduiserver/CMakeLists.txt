########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"soliduiserver\")

set(kded_soliduiserver_SRCS
   soliduiserver.cpp
   deviceactionsdialog.cpp
   deviceaction.cpp
   devicenothingaction.cpp
   deviceserviceaction.cpp
)

ki18n_wrap_ui(kded_soliduiserver_SRCS deviceactionsdialogview.ui)

add_library(soliduiserver MODULE ${kded_soliduiserver_SRCS})
kcoreaddons_desktop_to_json(soliduiserver soliduiserver.desktop)

target_link_libraries(soliduiserver KF5::Solid KF5::DBusAddons KF5::Wallet KF5::KIOCore KF5::WindowSystem KF5::KDELibs4Support)

install(TARGETS soliduiserver DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
