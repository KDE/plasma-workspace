project(plasma-systemtray)

add_subdirectory(plugin)
add_subdirectory(tests)

plasma_install_package(package org.kde.plasma.systemtray)

plasma_install_package(tests/systrayplasmoidtest org.kde.systrayplasmoidtest)
