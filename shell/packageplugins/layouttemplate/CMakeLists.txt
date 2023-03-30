add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_layouttemplate\")

kcoreaddons_add_plugin(plasma_packagestructure_layoutemplate SOURCES layouttemplate.cpp INSTALL_NAMESPACE "kf6/packagestructure")

target_link_libraries(plasma_packagestructure_layoutemplate
   KF6::I18n
   KF6::Package
)

set_target_properties(plasma_packagestructure_layoutemplate PROPERTIES OUTPUT_NAME plasma_layouttemplate)
