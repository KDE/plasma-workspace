set(switchdesktop_SRCS
    desktop.cpp
)

add_library(plasma_containmentactions_switchdesktop MODULE ${switchdesktop_SRCS})

kcoreaddons_desktop_to_json(plasma_containmentactions_switchdesktop plasma-containmentactions-switchdesktop.desktop)

target_link_libraries(plasma_containmentactions_switchdesktop
                        Qt::Widgets
                        KF5::Plasma
                        PW::LibTaskManager
                       )
install(TARGETS plasma_containmentactions_switchdesktop DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/containmentactions)
