add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_wallpaper\")

set(wallpaper_SRCS
    wallpaper.cpp
)

add_library(plasma_packagestructure_wallpaper MODULE ${wallpaper_SRCS})

target_link_libraries(plasma_packagestructure_wallpaper
   KF5::Declarative
   KF5::I18n
   KF5::Package
)

kcoreaddons_desktop_to_json(plasma_packagestructure_wallpaper plasma-packagestructure-wallpaper.desktop)

install(TARGETS plasma_packagestructure_wallpaper DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)
