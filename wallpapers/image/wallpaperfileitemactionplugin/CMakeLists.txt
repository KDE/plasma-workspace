# SPDX-FileCopyrightText: 2022 Julius Zint <julius@zint.sh>
# SPDX-License-Identifier: MIT
kcoreaddons_add_plugin(wallpaperfileitemaction
    SOURCES wallpaperfileitemaction.cpp wallpaperfileitemaction.h
    INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kfileitemaction")

target_link_libraries(wallpaperfileitemaction
    KF5::KIOWidgets
    KF5::I18n
)
