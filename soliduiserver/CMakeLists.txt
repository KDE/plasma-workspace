########### next target ###############

set(kded_soliduiserver_SRCS
   soliduiserver.cpp
   deviceactionsdialog.cpp
   deviceaction.cpp
   devicenothingaction.cpp
   deviceserviceaction.cpp
)

qt5_wrap_ui(kded_soliduiserver_SRCS deviceactionsdialogview.ui)

add_library(kded_soliduiserver MODULE ${kded_soliduiserver_SRCS})


target_link_libraries(kded_soliduiserver KF5::Solid KF5::DBusAddons KF5::KDELibs4Support KF5::Wallet KF5::KIOCore)

install(TARGETS kded_soliduiserver DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES soliduiserver.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

