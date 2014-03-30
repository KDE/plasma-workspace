project(plasma-containmentactions-switchwindow)

set(switchwindow_SRCS
    switch.cpp
)
kde4_add_ui_files(switchwindow_SRCS config.ui)

add_library(plasma_containmentactions_switchwindow MODULE ${switchwindow_SRCS})
target_link_libraries(plasma_containmentactions_switchwindow KF5::Plasma KIOCore)

install(TARGETS plasma_containmentactions_switchwindow DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-containmentactions-switchwindow.desktop DESTINATION ${SERVICES_INSTALL_DIR})
