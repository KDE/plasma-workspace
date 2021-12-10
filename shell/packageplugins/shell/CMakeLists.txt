add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_plasmashell\")

kcoreaddons_add_plugin(plasma_packagestructure_plasmashell SOURCES shellpackage.cpp INSTALL_NAMESPACE "kpackage/packagestructure")

target_link_libraries(plasma_packagestructure_plasmashell
   KF5::I18n
   KF5::Package
)

set_target_properties(plasma_packagestructure_plasmashell PROPERTIES OUTPUT_NAME plasma_shell)
