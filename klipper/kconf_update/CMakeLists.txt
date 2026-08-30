# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

# history2.lst -> history3.sqlite
add_executable(plasma6.3-update-clipboard-database-2-to-3)
target_sources(plasma6.3-update-clipboard-database-2-to-3 PRIVATE plasma6.3-update-clipboard-database-2-to-3.cpp)
target_link_libraries(plasma6.3-update-clipboard-database-2-to-3 PRIVATE Qt::Gui ZLIB::ZLIB klipper)
install(FILES plasma6.3-update-clipboard-database-2-to-3.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasma6.3-update-clipboard-database-2-to-3 DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)

# Meta+Ctrl+X stopped being the default shortcut for the automatic action menu
add_executable(plasma6.8-remove-action-menu-shortcut)
target_sources(plasma6.8-remove-action-menu-shortcut PRIVATE plasma6.8-remove-action-menu-shortcut.cpp)
target_link_libraries(plasma6.8-remove-action-menu-shortcut PRIVATE Qt::Gui KF6::GlobalAccel)
install(FILES plasma6.8-remove-action-menu-shortcut.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasma6.8-remove-action-menu-shortcut DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
