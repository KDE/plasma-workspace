# SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

add_executable(plasma6.0-remove-old-shortcuts)
target_sources(plasma6.0-remove-old-shortcuts PRIVATE plasma6.0-remove-old-shortcuts.cpp)
target_link_libraries(plasma6.0-remove-old-shortcuts PRIVATE KF6::ConfigCore KF6::GlobalAccel)

install(FILES plasma6.0-remove-old-shortcuts.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasma6.0-remove-old-shortcuts DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)

add_executable(plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels)
target_sources(plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels PRIVATE plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels.cpp)
target_link_libraries(plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels KF6::ConfigCore)

add_executable(plasmashell-6.0-keep-custom-position-of-panels)
target_sources(plasmashell-6.0-keep-custom-position-of-panels PRIVATE plasmashell-6.0-keep-custom-position-of-panels.cpp)
target_link_libraries(plasmashell-6.0-keep-custom-position-of-panels KF6::ConfigCore)

install(FILES plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)

install(FILES plasmashell-6.0-keep-custom-position-of-panels.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasmashell-6.0-keep-custom-position-of-panels DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
