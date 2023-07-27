# SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

add_executable(plasmashell-5.27-use-panel-thickness-in-default-group)
target_sources(plasmashell-5.27-use-panel-thickness-in-default-group PRIVATE plasmashell-5.27-use-panel-thickness-in-default-group.cpp)
target_link_libraries(plasmashell-5.27-use-panel-thickness-in-default-group PRIVATE KF6::ConfigCore)

install(FILES plasmashell-5.27-use-panel-thickness-in-default-group.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasmashell-5.27-use-panel-thickness-in-default-group DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)

add_executable(plasma6.0-remove-old-shortcuts)
target_sources(plasma6.0-remove-old-shortcuts PRIVATE plasma6.0-remove-old-shortcuts.cpp)
target_link_libraries(plasma6.0-remove-old-shortcuts PRIVATE KF6::ConfigCore KF6::GlobalAccel)

install(FILES plasma6.0-remove-old-shortcuts.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasma6.0-remove-old-shortcuts DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
