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

add_library(kded_soliduiserver MODULE ${kded_soliduiserver_SRCS})
kservice_desktop_to_json(kded_soliduiserver soliduiserver.desktop)

target_link_libraries(kded_soliduiserver KF5::Solid KF5::DBusAddons KF5::KDELibs4Support KF5::Wallet KF5::KIOCore)

install(TARGETS kded_soliduiserver DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES soliduiserver.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
