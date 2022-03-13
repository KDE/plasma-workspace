set(packagekit_engine_SRCS
    packagekitjob.cpp
    packagekitengine.cpp
    packagekitservice.cpp
    packagekitjob.h
    packagekitengine.h
    packagekitservice.h
)

kcoreaddons_add_plugin(plasma_engine_packagekit SOURCES ${packagekit_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

target_link_libraries(plasma_engine_packagekit KF5::Plasma KF5::CoreAddons Qt::DBus )

install(FILES packagekit.operations
    DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

