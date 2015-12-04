add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.symplesystray\")

plasma_install_package(package org.kde.plasma.simplesystray)

set(systemtray_SRCS
    systemtray.cpp
)

add_library(plasma_applet_systemtray MODULE ${systemtray_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_systemtray package/metadata.desktop)

target_link_libraries(plasma_applet_systemtray
                      Qt5::Gui
                      KF5::Plasma
                      Qt5::DBus
                      KF5::I18n)


install(TARGETS plasma_applet_systemtray DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)


set(systemtraycontainer_SRCS
    systemtraycontainer.cpp
)

add_library(plasma_applet_systemtraycontainer MODULE ${systemtraycontainer_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_systemtraycontainer containermetadata.desktop)

target_link_libraries(plasma_applet_systemtraycontainer
                      Qt5::Gui
                      Qt5::Quick
                      KF5::Plasma
                      KF5::I18n)


install(TARGETS plasma_applet_systemtraycontainer DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

install(FILES containermetadata.desktop DESTINATION ${SERVICES_INSTALL_DIR} RENAME plasma-applet-org.kde.plasma.systraycontainer.desktop)
install(FILES containermetadata.desktop DESTINATION ${PLASMA_DATA_INSTALL_DIR}/plasmoids/org.kde.plasma.systraycontainer RENAME metadata.desktop )
