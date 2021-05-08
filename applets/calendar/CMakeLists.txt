kcoreaddons_add_plugin(plasma_applet_calendar SOURCES calendarapplet.cpp INSTALL_NAMESPACE "plasma/applets")
kcoreaddons_desktop_to_json(plasma_applet_calendar package/metadata.desktop)

target_link_libraries(plasma_applet_calendar
                      KF5::Plasma)

plasma_install_package(package org.kde.plasma.calendar)
