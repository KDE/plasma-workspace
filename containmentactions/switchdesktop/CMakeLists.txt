kcoreaddons_add_plugin(plasma_containmentactions_switchdesktop SOURCES desktop.cpp desktop.h INSTALL_NAMESPACE "plasma/containmentactions")

set_target_properties(plasma_containmentactions_switchdesktop PROPERTIES
    OUTPUT_NAME org.kde.switchdesktop
)

target_link_libraries(plasma_containmentactions_switchdesktop
                        Qt::Widgets
                        Plasma::Plasma
                        PW::LibTaskManager
                       )

