include_directories (${CMAKE_CURRENT_BINARY_DIR}/..)

kcoreaddons_add_plugin(colorcorrectlocationupdater SOURCES locationupdater.cpp locationupdater.h INSTALL_NAMESPACE "kf6/kded")

target_link_libraries(colorcorrectlocationupdater
                        KF6::DBusAddons
                        KF6::CoreAddons
                        colorcorrect)
