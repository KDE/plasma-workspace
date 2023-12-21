set(apps_engine_SRCS
    appsengine.cpp
    appsource.cpp
    appservice.cpp
    appjob.cpp
    appsengine.h
    appsource.h
    appservice.h
    appjob.h
)

add_library(plasma_engine_apps MODULE ${apps_engine_SRCS})

target_link_libraries(plasma_engine_apps
    Plasma::Plasma
    Plasma::Plasma5Support
    KF6::Service
    KF6::KIOCore
    KF6::KIOWidgets
)

install(TARGETS plasma_engine_apps DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)
install(FILES apps.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
