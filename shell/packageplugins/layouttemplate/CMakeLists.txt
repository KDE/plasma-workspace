add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_layouttemplate\")

set(layouttemplate_SRCS
    layouttemplate.cpp
)

add_library(plasma_packagestructure_layoutemplate MODULE ${layouttemplate_SRCS})

target_link_libraries(plasma_packagestructure_layoutemplate
   KF5::I18n
   KF5::Package
)

kcoreaddons_desktop_to_json(plasma_packagestructure_layoutemplate plasma-packagestructure-layouttemplate.desktop)

install(TARGETS plasma_packagestructure_layoutemplate DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)
