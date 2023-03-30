add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_wallpaper\")

kcoreaddons_add_plugin(plasma_packagestructure_wallpaper SOURCES wallpaper.cpp INSTALL_NAMESPACE "kf6/packagestructure")

target_link_libraries(plasma_packagestructure_wallpaper
   KF6::I18n
   KF6::Package
)

set_target_properties(plasma_packagestructure_wallpaper PROPERTIES OUTPUT_NAME plasma_wallpaper)
