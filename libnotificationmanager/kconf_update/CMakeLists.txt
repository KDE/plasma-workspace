# SPDX-FileCopyrightText: 2025 Kristen McWilliam <kmcwilliampublic@gmail.com>
# SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

add_executable(plasma6.4-migrate-fullscreen-notifications-to-dnd)
target_sources(plasma6.4-migrate-fullscreen-notifications-to-dnd PRIVATE plasma6.4-migrate-fullscreen-notifications-to-dnd.cpp)
target_link_libraries(plasma6.4-migrate-fullscreen-notifications-to-dnd PRIVATE KF6::ConfigCore Qt::Core)
install(FILES plasma6.4-migrate-fullscreen-notifications-to-dnd.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasma6.4-migrate-fullscreen-notifications-to-dnd DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
