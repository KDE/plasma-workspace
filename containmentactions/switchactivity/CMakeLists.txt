project(plasma-containmentactions-switchactivity)

set(switchactivity_SRCS
    switch.cpp
)

add_library(plasma_containmentactions_switchactivity MODULE ${switchactivity_SRCS})
target_link_libraries(plasma_containmentactions_switchactivity KF5::Plasma KIOCore PW::KWorkspace)

install(TARGETS plasma_containmentactions_switchactivity DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-containmentactions-switchactivity.desktop DESTINATION ${SERVICES_INSTALL_DIR})
