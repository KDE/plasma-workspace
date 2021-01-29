add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.private.systemtray\")

plasma_install_package(package org.kde.plasma.private.systemtray)

set(systemtray_SRCS
    dbusserviceobserver.cpp
    plasmoidregistry.cpp
    systemtraysettings.cpp
    systemtraymodel.cpp
    sortedsystemtraymodel.cpp
    systemtray.cpp
)

ecm_qt_declare_logging_category(systemtray_SRCS HEADER debug.h
                                            IDENTIFIER SYSTEM_TRAY
                                            CATEGORY_NAME kde.systemtray
                                            DEFAULT_SEVERITY Info)

add_library(org.kde.plasma.private.systemtray MODULE ${systemtray_SRCS})

kcoreaddons_desktop_to_json(org.kde.plasma.private.systemtray package/metadata.desktop)

target_link_libraries(org.kde.plasma.private.systemtray
                      Qt::Gui
                      Qt::Quick
                      Qt::DBus
                      KF5::Plasma
                      KF5::XmlGui
                      KF5::I18n
                      KF5::ItemModels)

install(TARGETS org.kde.plasma.private.systemtray DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

add_subdirectory(container)
if(BUILD_TESTING)
  add_subdirectory(autotests)
  add_subdirectory(tests)
endif()
