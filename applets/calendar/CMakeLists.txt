set(calendarapplet_SRCS
    calendarapplet.cpp
)

add_library(plasma_applet_calendar MODULE ${calendarapplet_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_calendar package/metadata.desktop)

target_link_libraries(plasma_applet_calendar
                      KF5::Plasma)

install(TARGETS plasma_applet_calendar DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

plasma_install_package(package org.kde.plasma.calendar)
