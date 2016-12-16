set(iconapplet_SRCS
    iconapplet.cpp
)

add_library(plasma_applet_icon MODULE ${iconapplet_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_icon package/metadata.desktop)

target_link_libraries(plasma_applet_icon
                      KF5::I18n
                      KF5::KIOWidgets # for KRun
                      KF5::Plasma)

install(TARGETS plasma_applet_icon DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/applets)

plasma_install_package(package org.kde.plasma.icon)
