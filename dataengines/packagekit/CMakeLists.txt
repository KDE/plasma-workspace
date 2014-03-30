project(packagekitengine)

set(packagekit_engine_SRCS
    packagekitjob.cpp
    packagekitengine.cpp
    packagekitservice.cpp
)

add_library(plasma_engine_packagekit MODULE ${packagekit_engine_SRCS})
target_link_libraries(plasma_engine_packagekit KF5::Plasma KF5::CoreAddons )

install(TARGETS plasma_engine_packagekit
    DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)

install(FILES plasma-dataengine-packagekit.desktop
    DESTINATION ${SERVICES_INSTALL_DIR} )

install(FILES packagekit.operations
    DESTINATION ${DATA_INSTALL_DIR}/plasma/services)

