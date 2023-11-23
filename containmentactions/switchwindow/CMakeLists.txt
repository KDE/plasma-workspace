add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_switchwindow\")

set(switchwindow_SRCS
    switch.cpp
    switch.h
)
ki18n_wrap_ui(switchwindow_SRCS config.ui)

kcoreaddons_add_plugin(plasma_containmentactions_switchwindow SOURCES ${switchwindow_SRCS} INSTALL_NAMESPACE "plasma/containmentactions")

set_target_properties(plasma_containmentactions_switchwindow PROPERTIES
    OUTPUT_NAME switchwindow
)

target_link_libraries(plasma_containmentactions_switchwindow
                      Qt::Widgets
                      Plasma::Plasma
                      KF6::I18n
                      PW::LibTaskManager)
