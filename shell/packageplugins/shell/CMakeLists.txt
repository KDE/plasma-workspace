add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_plasmashell\")

set(shellpackage_SRCS
    shellpackage.cpp
)

add_library(plasma_packagestructure_plasmashell MODULE ${shellpackage_SRCS})

target_link_libraries(plasma_packagestructure_plasmashell
   KF5::I18n
   KF5::Package
)

kcoreaddons_desktop_to_json(plasma_packagestructure_plasmashell plasma-packagestructure-plasma-shell.desktop)

install(TARGETS plasma_packagestructure_plasmashell DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)
