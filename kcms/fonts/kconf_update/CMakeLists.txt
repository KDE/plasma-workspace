# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

add_executable(plasma6.0-remove-dpi-settings)
target_sources(plasma6.0-remove-dpi-settings PRIVATE plasma6.0-remove-dpi-settings.cpp)
target_link_libraries(plasma6.0-remove-dpi-settings PRIVATE KF6::ConfigCore)

install(FILES plasma6.0-remove-dpi-settings.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasma6.0-remove-dpi-settings DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
