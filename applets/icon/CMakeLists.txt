add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.icon\")

kcoreaddons_add_plugin(org.kde.plasma.icon SOURCES iconapplet.cpp iconapplet.h INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(org.kde.plasma.icon
                      KF6::I18n
                      KF6::KIOCore # for OpenFileManagerWindowJob
                      KF6::KIOGui # for FavIconRequestJob
                      KF6::KIOWidgets # for KRun
                      KF6::Notifications
                      KF6::Plasma
                      PW::LibTaskManager)

plasma_install_package(package org.kde.plasma.icon)
