project(plasma-containmentactions-minimalcontextmenu)

set(contextmenu_SRCS
    menu.cpp
)

add_library(plasma_containmentactions_minimalcontextmenu MODULE ${contextmenu_SRCS})

kcoreaddons_desktop_to_json(plasma_containmentactions_minimalcontextmenu plasma-containmentactions-minimalcontextmenu.desktop)

target_link_libraries(plasma_containmentactions_minimalcontextmenu
                      KF5::Plasma
                      KF5::KIOCore
                      KF5::XmlGui)

install(TARGETS plasma_containmentactions_minimalcontextmenu DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-containmentactions-minimalcontextmenu.desktop DESTINATION ${SERVICES_INSTALL_DIR})
