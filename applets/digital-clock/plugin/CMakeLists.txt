add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.digitalclock\")

find_package(IsoCodes)
set_package_properties(IsoCodes PROPERTIES
                       PURPOSE "Translation of country names in digital clock applet"
                       TYPE RUNTIME
                      )

set(digitalclockplugin_SRCS
    timezonemodel.cpp
    timezonesi18n.cpp
    digitalclockplugin.cpp
    clipboardmenu.cpp
    applicationintegration.cpp
    timezonemodel.h
    timezonesi18n.h
    digitalclockplugin.h
    clipboardmenu.h
    applicationintegration.h
    )

add_library(digitalclockplugin SHARED ${digitalclockplugin_SRCS})
target_link_libraries(digitalclockplugin
    PRIVATE
        Qt::Core
        Qt::Qml
        Qt::Widgets # for QAction...
        KF5::CoreAddons
        KF5::KIOGui
        KF5::Service
        KF5::I18n)

install(TARGETS digitalclockplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/digitalclock)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/digitalclock)
