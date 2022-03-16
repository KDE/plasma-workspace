include_directories (${CMAKE_CURRENT_BINARY_DIR}/..)

kcoreaddons_add_plugin(colorcorrectlocationupdater SOURCES locationupdater.cpp locationupdater.h INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kded")

target_link_libraries(colorcorrectlocationupdater
                        KF5::DBusAddons
                        KF5::CoreAddons
                        colorcorrect)
