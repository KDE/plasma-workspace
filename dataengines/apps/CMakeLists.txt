set(apps_engine_SRCS
    appsengine.cpp
    appsource.cpp
    appservice.cpp
    appjob.cpp
)

add_library(plasma_engine_apps MODULE ${apps_engine_SRCS})

target_link_libraries(plasma_engine_apps
    KF5::Plasma
    KF5::Service
    KF5::KIOCore
    KF5::KIOWidgets
)

kcoreaddons_desktop_to_json(plasma_engine_apps plasma-dataengine-apps.desktop)

install(TARGETS plasma_engine_apps DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-apps.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES apps.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
