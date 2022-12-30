# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_wallpaper_org.kde.image\")

set(plasma-apply-wallpaperimage_SRCS
    plasma-apply-wallpaperimage.cpp
)
add_executable(plasma-apply-wallpaperimage ${plasma-apply-wallpaperimage_SRCS})
target_link_libraries(plasma-apply-wallpaperimage
    Qt::Core
    Qt::DBus
    KF5::I18n
)

add_subdirectory(plugin)

install(TARGETS plasma-apply-wallpaperimage DESTINATION ${KDE_INSTALL_BINDIR})

install(FILES wallpaper.knsrc wallpaper-mobile.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

plasma_install_package(imagepackage org.kde.image wallpapers wallpaper)
plasma_install_package(slideshowpackage org.kde.slideshow wallpapers wallpaper)

configure_file(setaswallpaper.desktop.in setaswallpaper.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/setaswallpaper.desktop DESTINATION ${KDE_INSTALL_DATADIR}/kio/servicemenus)

install(DIRECTORY imagepackage/contents/ui DESTINATION ${PLASMA_DATA_INSTALL_DIR}/wallpapers/org.kde.slideshow/contents PATTERN .svn EXCLUDE PATTERN CMakeLists.txt EXCLUDE PATTERN Messages.sh EXCLUDE)
