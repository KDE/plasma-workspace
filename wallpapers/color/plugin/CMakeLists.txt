set(color_SRCS
    imagecolors.cpp
)

add_library(plasma_wallpaper_colorplugin_static STATIC ${color_SRCS})

target_link_libraries(plasma_wallpaper_colorplugin_static
    Qt::Core
    Qt::Quick
    Qt::Qml
    pthread
)

add_library(plasma_wallpaper_colorplugin SHARED colorplugin.cpp)
target_link_libraries(plasma_wallpaper_colorplugin
    plasma_wallpaper_colorplugin_static
)

install(TARGETS plasma_wallpaper_colorplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/color)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/color)
