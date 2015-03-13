add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_lookandfeel\")

set(lookandfeel_SRCS
    lookandfeel.cpp
)

add_library(plasma_packagestructure_lookandfeel MODULE ${lookandfeel_SRCS})
kcoreaddons_desktop_to_json(plasma_packagestructure_lookandfeel plasma-packagestructure-lookandfeel.desktop)

target_link_libraries(plasma_packagestructure_lookandfeel
   KF5::I18n
   KF5::Package
)

install(TARGETS plasma_packagestructure_lookandfeel DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)
