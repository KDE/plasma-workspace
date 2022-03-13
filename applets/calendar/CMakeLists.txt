kcoreaddons_add_plugin(plasma_applet_calendar SOURCES calendarapplet.cpp calendarapplet.h INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(plasma_applet_calendar
                      KF5::Plasma)

plasma_install_package(package org.kde.plasma.calendar)
