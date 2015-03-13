plasma_install_package(package org.kde.plasma.icon)

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.icon\")

set(iconplugin_SRCS
  plugin/icon_p.cpp
  plugin/iconplugin.cpp
)

install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/icon)

add_library(iconplugin SHARED ${iconplugin_SRCS})

target_link_libraries(iconplugin
  Qt5::Core
  Qt5::Qml
  Qt5::Quick
  KF5::KIOCore # KFileItem
  KF5::KIOWidgets # KRun
)

install(TARGETS iconplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/icon)
