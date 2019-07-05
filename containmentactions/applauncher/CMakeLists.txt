add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_applauncher\")

set(applauncher_SRCS
    launch.cpp
)
ki18n_wrap_ui(applauncher_SRCS config.ui)

add_library(plasma_containmentactions_applauncher MODULE ${applauncher_SRCS})

kcoreaddons_desktop_to_json(plasma_containmentactions_applauncher plasma-containmentactions-applauncher.desktop)

target_link_libraries(plasma_containmentactions_applauncher KF5::Plasma KF5::KIOCore KF5::KIOWidgets KF5::I18n)

install(TARGETS plasma_containmentactions_applauncher DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/containmentactions)
