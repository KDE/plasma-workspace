add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_applauncher\")

set(applauncher_SRCS
    launch.cpp
)
ki18n_wrap_ui(applauncher_SRCS config.ui)

kcoreaddons_add_plugin(plasma_containmentactions_applauncher SOURCES ${applauncher_SRCS} INSTALL_NAMESPACE "plasma/containmentactions")

set_target_properties(plasma_containmentactions_applauncher PROPERTIES
    OUTPUT_NAME org.kde.applauncher
)

target_link_libraries(plasma_containmentactions_applauncher Plasma::Plasma KF6::KIOCore KF6::KIOWidgets KF6::I18n)
