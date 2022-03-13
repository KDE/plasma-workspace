add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_switchwindow\")

set(switchwindow_SRCS
    switch.cpp
    switch.h
)
ki18n_wrap_ui(switchwindow_SRCS config.ui)

kcoreaddons_add_plugin(plasma_containmentactions_switchwindow SOURCES ${switchwindow_SRCS} INSTALL_NAMESPACE "plasma/containmentactions")

target_link_libraries(plasma_containmentactions_switchwindow
                      Qt::Widgets
                      KF5::Plasma
                      KF5::I18n
                      PW::LibTaskManager)
