add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_wallpaperimages\")

set(wallpaper_SRCS
    wallpaperpackage.cpp
)

add_library(plasma_packagestructure_wallpaperimages MODULE ${wallpaper_SRCS})

target_link_libraries(plasma_packagestructure_wallpaperimages
   KF5::I18n
   KF5::Package
)

kcoreaddons_desktop_to_json(plasma_packagestructure_wallpaperimages plasma-packagestructure-wallpaperimages.desktop)

install(TARGETS plasma_packagestructure_wallpaperimages DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)
