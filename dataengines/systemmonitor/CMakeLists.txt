
set(systemmonitor_engine_SRCS
   systemmonitor.cpp
)

add_library(plasma_engine_systemmonitor MODULE ${systemmonitor_engine_SRCS})

target_link_libraries(plasma_engine_systemmonitor
    Qt5::Network
    KF5::Plasma
    KF5::Service
    KF5::SysGuard
    KF5::KDE4Support
)

kservice_desktop_to_json(plasma_engine_systemmonitor plasma-dataengine-systemmonitor.desktop)

install(TARGETS plasma_engine_systemmonitor DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-systemmonitor.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
