add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.digitalclock\")

find_package(IsoCodes)
set_package_properties(IsoCodes PROPERTIES DESCRIPTION "ISO language, territory, currency, script codes and their translations"
                       URL "https://salsa.debian.org/iso-codes-team/iso-codes"
                       PURPOSE "Translation of country names in digital clock applet"
                       TYPE RUNTIME
                      )

set(digitalclockplugin_SRCS
    timezonemodel.cpp
    timezonesi18n.cpp
    digitalclockplugin.cpp
    clipboardmenu.cpp
    )

add_library(digitalclockplugin SHARED ${digitalclockplugin_SRCS})
target_link_libraries(digitalclockplugin Qt5::Core
                                          Qt5::Qml
                                          Qt5::Widgets # for QAction...
                                          KF5::CoreAddons
                                          KF5::I18n)

install(TARGETS digitalclockplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/digitalclock)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/digitalclock)
