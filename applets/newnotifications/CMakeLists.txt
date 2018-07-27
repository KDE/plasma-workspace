add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.notifications\")

set(notificationapplet_SRCS
    notificationapplet.cpp
)

add_library(plasma_applet_newnotifications MODULE ${notificationapplet_SRCS}) # FIXME

kcoreaddons_desktop_to_json(plasma_applet_newnotifications package/metadata.desktop) # FIXME

target_link_libraries(plasma_applet_newnotifications  # FIXME
                      # shouldnt require it as it's all QML, isn't it?
                      #PW::NotificationManager
                      KF5::I18n
                      KF5::Plasma)

install(TARGETS plasma_applet_newnotifications DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)  # FIXME

plasma_install_package(package org.kde.plasma.newnotifications)
