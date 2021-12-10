add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_applauncher\")

set(applauncher_SRCS
    launch.cpp
)
ki18n_wrap_ui(applauncher_SRCS config.ui)

kcoreaddons_add_plugin(plasma_containmentactions_applauncher SOURCES ${applauncher_SRCS} INSTALL_NAMESPACE "plasma/containmentactions")

target_link_libraries(plasma_containmentactions_applauncher KF5::Plasma KF5::KIOCore KF5::KIOWidgets KF5::I18n)
