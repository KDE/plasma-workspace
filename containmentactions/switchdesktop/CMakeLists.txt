kcoreaddons_add_plugin(plasma_containmentactions_switchdesktop SOURCES desktop.cpp INSTALL_NAMESPACE "plasma/containmentactions")
kcoreaddons_desktop_to_json(plasma_containmentactions_switchdesktop plasma-containmentactions-switchdesktop.desktop)

target_link_libraries(plasma_containmentactions_switchdesktop
                        Qt::Widgets
                        KF5::Plasma
                        PW::LibTaskManager
                       )

