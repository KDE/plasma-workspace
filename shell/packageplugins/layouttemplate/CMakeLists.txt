add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_layouttemplate\")

set(layouttemplate_SRCS
    layouttemplate.cpp
)

add_library(plasma_packagestructure_layoutemplate MODULE ${layouttemplate_SRCS})

target_link_libraries(plasma_packagestructure_layoutemplate
   KF5::I18n
   KF5::Plasma
)

kservice_desktop_to_json(plasma_packagestructure_layoutemplate plasma-packagestructure-layouttemplate.desktop)

install(TARGETS plasma_packagestructure_layoutemplate DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/packagestructure)
