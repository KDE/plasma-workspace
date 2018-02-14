include_directories (${CMAKE_CURRENT_BINARY_DIR}/..)

add_library(colorcorrectlocationupdater MODULE locationupdater.cpp)

kcoreaddons_desktop_to_json(colorcorrectlocationupdater colorcorrectlocationupdater.desktop)
target_link_libraries(colorcorrectlocationupdater
                        KF5::DBusAddons
                        KF5::CoreAddons
                        colorcorrect)

install(TARGETS colorcorrectlocationupdater DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
