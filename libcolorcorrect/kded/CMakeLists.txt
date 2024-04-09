include_directories (${CMAKE_CURRENT_BINARY_DIR}/..)

kcoreaddons_add_plugin(colorcorrectlocationupdater SOURCES locationupdater.cpp locationupdater.h INSTALL_NAMESPACE "kf6/kded")

target_link_libraries(colorcorrectlocationupdater
                        KF6::DBusAddons
                        KF6::CoreAddons
                        colorcorrect)

ecm_qt_declare_logging_category(colorcorrectlocationupdater
                        HEADER locationupdater_debug.h
                        IDENTIFIER LOCATIONUPDATER
                        CATEGORY_NAME org.kde.colorcorrectlocationupdater
                        DESCRIPTION "kded module providing location updates for Night Light"
                        EXPORT PLASMAWORKSPACE
    )
