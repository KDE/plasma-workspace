add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_wallpaperimages\")

kcoreaddons_add_plugin(plasma_packagestructure_wallpaperimages SOURCES wallpaperpackage.cpp INSTALL_NAMESPACE "kpackage/packagestructure")

target_link_libraries(plasma_packagestructure_wallpaperimages
   KF5::I18n
   KF5::Package
)

set_target_properties(plasma_packagestructure_wallpaperimages PROPERTIES OUTPUT_NAME wallpaper_images)
