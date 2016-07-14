add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_switchwindow\")

set(switchwindow_SRCS
    switch.cpp
)
ki18n_wrap_ui(switchwindow_SRCS config.ui)

add_library(plasma_containmentactions_switchwindow MODULE ${switchwindow_SRCS})

kcoreaddons_desktop_to_json(plasma_containmentactions_switchwindow plasma-containmentactions-switchwindow.desktop)

target_link_libraries(plasma_containmentactions_switchwindow
                      KF5::Plasma
                      KF5::KIOCore
                      KF5::I18n
                      KF5::WindowSystem
                      taskmanager)

install(TARGETS plasma_containmentactions_switchwindow DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-containmentactions-switchwindow.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
