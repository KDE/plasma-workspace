add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.icon\")

kcoreaddons_add_plugin(plasma_applet_icon SOURCES iconapplet.cpp INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(plasma_applet_icon
                      KF5::I18n
                      KF5::KIOCore # for OpenFileManagerWindowJob
                      KF5::KIOGui # for FavIconRequestJob
                      KF5::KIOWidgets # for KRun
                      KF5::Notifications
                      KF5::Plasma
                      PW::LibTaskManager)

plasma_install_package(package org.kde.plasma.icon)
