plasma_install_package(package org.kde.plasma.icon)

set(iconplugin_SRCS
  plugin/icon_p.cpp
  plugin/iconplugin.cpp
)

install(FILES plugin/qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/icon)

add_library(iconplugin SHARED ${iconplugin_SRCS})

target_link_libraries(iconplugin
  Qt5::Core
  Qt5::Qml
  Qt5::Quick
  KF5::KIOCore # KFileItem
)

install(TARGETS iconplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/icon)
