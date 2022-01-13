add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_layouttemplate\")

kcoreaddons_add_plugin(plasma_packagestructure_layoutemplate SOURCES layouttemplate.cpp INSTALL_NAMESPACE "kpackage/packagestructure")

target_link_libraries(plasma_packagestructure_layoutemplate
   KF5::I18n
   KF5::Package
)

set_target_properties(plasma_packagestructure_layoutemplate PROPERTIES OUTPUT_NAME plasma_layouttemplate)
