# SPDX-FileCopyrightText: 2022 Julius Zint <julius@zint.sh>
# SPDX-License-Identifier: MIT
kcoreaddons_add_plugin(wallpaperfileitemaction
    SOURCES wallpaperfileitemaction.cpp wallpaperfileitemaction.h
    INSTALL_NAMESPACE "kf6/kfileitemaction")

target_link_libraries(wallpaperfileitemaction
    KF6::KIOWidgets
    KF6::I18n
)
