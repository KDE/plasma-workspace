add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_wallpaper\")

kcoreaddons_add_plugin(plasma_packagestructure_wallpaper SOURCES wallpaper.cpp INSTALL_NAMESPACE "kpackage/packagestructure")

target_link_libraries(plasma_packagestructure_wallpaper
   KF5::I18n
   KF5::Package
)

set_target_properties(plasma_packagestructure_wallpaper PROPERTIES OUTPUT_NAME plasma_wallpaper)
