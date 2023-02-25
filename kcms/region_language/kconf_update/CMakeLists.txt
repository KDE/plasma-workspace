# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

add_executable(plasmashell-6.0-rename-plasma-localerc-to-plasmalocalerc)
target_sources(plasmashell-6.0-rename-plasma-localerc-to-plasmalocalerc PRIVATE plasmashell-6.0-rename-plasma-localerc-to-plasmalocalerc.cpp)
target_link_libraries(plasmashell-6.0-rename-plasma-localerc-to-plasmalocalerc PRIVATE Qt::Core)

install(FILES plasmashell-6.0-rename-plasma-localerc-to-plasmalocalerc.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasmashell-6.0-rename-plasma-localerc-to-plasmalocalerc DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
