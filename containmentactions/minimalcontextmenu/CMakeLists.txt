project(plasma-containmentactions-minimalcontextmenu)

set(contextmenu_SRCS
    menu.cpp
)

add_library(plasma_containmentactions_minimalcontextmenu MODULE ${contextmenu_SRCS})
target_link_libraries(plasma_containmentactions_minimalcontextmenu KF5::Plasma KIOCore)

install(TARGETS plasma_containmentactions_minimalcontextmenu DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-containmentactions-minimalcontextmenu.desktop DESTINATION ${SERVICES_INSTALL_DIR})
