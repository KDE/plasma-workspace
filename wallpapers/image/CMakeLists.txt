add_definitions(-DTRANSLATION_DOMAIN=\"plasma_wallpaper_org.kde.image\")

set(plasma-apply-wallpaperimage_SRCS
    plasma-apply-wallpaperimage.cpp
)
add_executable(plasma-apply-wallpaperimage ${plasma-apply-wallpaperimage_SRCS})
target_link_libraries(plasma-apply-wallpaperimage
    Qt::Core
    Qt::DBus
    KF6::I18n
)

add_subdirectory(plugin)
add_subdirectory(wallpaperfileitemactionplugin)

install(TARGETS plasma-apply-wallpaperimage DESTINATION ${KDE_INSTALL_BINDIR})

install(FILES wallpaper.knsrc wallpaper-mobile.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

plasma_install_package(imagepackage org.kde.image wallpapers wallpaper)
plasma_install_package(slideshowpackage org.kde.slideshow wallpapers wallpaper)

# Generate the default wallpaper path
configure_file(main.xml.in ${CMAKE_CURRENT_BINARY_DIR}/main.xml @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/main.xml DESTINATION ${PLASMA_DATA_INSTALL_DIR}/wallpapers/org.kde.image/contents/config)
install(DIRECTORY imagepackage/contents/ui DESTINATION ${PLASMA_DATA_INSTALL_DIR}/wallpapers/org.kde.slideshow/contents PATTERN .svn EXCLUDE PATTERN CMakeLists.txt EXCLUDE PATTERN Messages.sh EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/main.xml DESTINATION ${PLASMA_DATA_INSTALL_DIR}/wallpapers/org.kde.slideshow/contents/config)
