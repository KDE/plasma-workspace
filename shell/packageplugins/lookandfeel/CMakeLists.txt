add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_lookandfeel\")

set(lookandfeel_SRCS
    lookandfeel.cpp
)

add_library(plasma_packagestructure_lookandfeel MODULE ${lookandfeel_SRCS})
kservice_desktop_to_json(plasma_packagestructure_lookandfeel plasma-packagestructure-lookandfeel.desktop)

target_link_libraries(plasma_packagestructure_lookandfeel
   KF5::I18n
   KF5::Plasma
)

install(TARGETS plasma_packagestructure_lookandfeel DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/packagestructure)
