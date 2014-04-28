
set(systemmonitor_engine_SRCS
   systemmonitor.cpp
)

add_library(plasma_engine_systemmonitor MODULE ${systemmonitor_engine_SRCS})

target_link_libraries(plasma_engine_systemmonitor
    Qt5::Network
    KF5::Plasma
    KF5::Service
    KF5::SysGuard
    KF5::KDELibs4Support
)

kservice_desktop_to_json(plasma_engine_systemmonitor plasma-dataengine-systemmonitor.desktop)

install(TARGETS plasma_engine_systemmonitor DESTINATION ${PLASMA_DATA_INSTALL_DIR}/dataengine)
install(FILES plasma-dataengine-systemmonitor.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
