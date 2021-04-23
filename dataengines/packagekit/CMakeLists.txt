set(packagekit_engine_SRCS
    packagekitjob.cpp
    packagekitengine.cpp
    packagekitservice.cpp
)

add_library(plasma_engine_packagekit MODULE ${packagekit_engine_SRCS})

kcoreaddons_desktop_to_json(plasma_engine_packagekit plasma-dataengine-packagekit.desktop)

target_link_libraries(plasma_engine_packagekit KF5::Plasma5Support KF5::CoreAddons Qt::DBus )

install(TARGETS plasma_engine_packagekit
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)

install(FILES plasma-dataengine-packagekit.desktop
    DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

install(FILES packagekit.operations
    DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)

