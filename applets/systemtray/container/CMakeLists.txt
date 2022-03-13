
plasma_install_package(package org.kde.plasma.systemtray)

set(systemtraycontainer_SRCS
    systemtraycontainer.cpp
    systemtraycontainer.h
)

ecm_qt_declare_logging_category(systemtraycontainer_SRCS HEADER debug.h
                                            IDENTIFIER SYSTEM_TRAY_CONTAINER
                                            CATEGORY_NAME kde.systemtraycontainer
                                            DEFAULT_SEVERITY Info)

kcoreaddons_add_plugin(org.kde.plasma.systemtray SOURCES ${systemtraycontainer_SRCS} INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(org.kde.plasma.systemtray
                      Qt::Gui
                      Qt::Quick
                      KF5::Plasma
                      KF5::XmlGui
                      KF5::I18n)
