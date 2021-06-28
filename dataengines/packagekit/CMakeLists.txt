set(packagekit_engine_SRCS
    packagekitjob.cpp
    packagekitengine.cpp
    packagekitservice.cpp
)

kcoreaddons_add_plugin(plasma_engine_packagekit SOURCES ${packagekit_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

kcoreaddons_desktop_to_json(plasma_engine_packagekit plasma-dataengine-packagekit.desktop)

target_link_libraries(plasma_engine_packagekit KF5::Plasma KF5::CoreAddons Qt::DBus )

install(FILES packagekit.operations
    DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

