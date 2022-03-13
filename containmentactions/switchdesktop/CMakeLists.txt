kcoreaddons_add_plugin(plasma_containmentactions_switchdesktop SOURCES desktop.cpp desktop.h INSTALL_NAMESPACE "plasma/containmentactions")

target_link_libraries(plasma_containmentactions_switchdesktop
                        Qt::Widgets
                        KF5::Plasma
                        PW::LibTaskManager
                       )

