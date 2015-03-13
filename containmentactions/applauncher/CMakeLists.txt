set(applauncher_SRCS
    launch.cpp
)

add_library(plasma_containmentactions_applauncher MODULE ${applauncher_SRCS})

kcoreaddons_desktop_to_json(plasma_containmentactions_applauncher plasma-containmentactions-applauncher.desktop)

target_link_libraries(plasma_containmentactions_applauncher KF5::Plasma KF5::KIOCore KF5::KIOWidgets)

install(TARGETS plasma_containmentactions_applauncher DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-containmentactions-applauncher.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
