include_directories (${CMAKE_CURRENT_BINARY_DIR}/..)

kcoreaddons_add_plugin(colorcorrectlocationupdater SOURCES locationupdater.cpp INSTALL_NAMESPACE "kf5/kded")

kcoreaddons_desktop_to_json(colorcorrectlocationupdater colorcorrectlocationupdater.desktop)
target_link_libraries(colorcorrectlocationupdater
                        KF5::DBusAddons
                        KF5::CoreAddons
                        colorcorrect)
