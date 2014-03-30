project(plasma-containmentactions-switchdesktop)

set(switchdesktop_SRCS
    desktop.cpp
)

add_library(plasma_containmentactions_switchdesktop MODULE ${switchdesktop_SRCS})

kservice_desktop_to_json(plasma_containmentactions_switchdesktop plasma-containmentactions-switchdesktop.desktop)

target_link_libraries(plasma_containmentactions_switchdesktop
                        KF5::Plasma
                        KF5::WindowSystem
                        KF5::I18n
			KF5::KIOCore
                       )

install(TARGETS plasma_containmentactions_switchdesktop DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-containmentactions-switchdesktop.desktop DESTINATION ${SERVICES_INSTALL_DIR})
