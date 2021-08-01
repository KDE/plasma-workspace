add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_wallpaper\")

kcoreaddons_add_plugin(plasma_packagestructure_wallpaper SOURCES wallpaper.cpp INSTALL_NAMESPACE "kpackage/packagestructure")

target_link_libraries(plasma_packagestructure_wallpaper
   KF5::Declarative
   KF5::I18n
   KF5::Package
)

kcoreaddons_desktop_to_json(plasma_packagestructure_wallpaper plasma-packagestructure-wallpaper.desktop)
set_target_properties(plasma_packagestructure_wallpaper PROPERTIES OUTPUT_NAME plasma_wallpaper)
