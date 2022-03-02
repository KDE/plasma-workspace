kcoreaddons_add_plugin(org.kde.plasma.calendar SOURCES calendarapplet.cpp calendarapplet.h INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(org.kde.plasma.calendar
                      KF5::Plasma)

plasma_install_package(package org.kde.plasma.calendar)
