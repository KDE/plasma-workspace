add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.appmenu\")

add_subdirectory(lib)
add_subdirectory(plugin)

plasma_install_package(package org.kde.plasma.appmenu)
