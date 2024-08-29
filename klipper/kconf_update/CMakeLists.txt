# SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
# SPDX-License-Identifier: LGPL-2.0-or-later

add_executable(plasma6.3-klipper-mimedatamap)
target_sources(plasma6.3-klipper-mimedatamap PRIVATE plasma6.3-klipper-mimedatamap.cpp)
target_link_libraries(plasma6.3-klipper-mimedatamap PRIVATE Qt6::Core KF6::CoreAddons klipper ${ZLIB_LIBRARY})

install(FILES plasma6.3-klipper-mimedatamap.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasma6.3-klipper-mimedatamap DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
