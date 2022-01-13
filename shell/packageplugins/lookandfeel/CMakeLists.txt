add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_lookandfeel\")

kcoreaddons_add_plugin(plasma_packagestructure_lookandfeel SOURCES lookandfeel.cpp INSTALL_NAMESPACE "kpackage/packagestructure")

target_link_libraries(plasma_packagestructure_lookandfeel
   KF5::I18n
   KF5::Package
)
set_target_properties(plasma_packagestructure_lookandfeel PROPERTIES OUTPUT_NAME plasma_lookandfeel)
