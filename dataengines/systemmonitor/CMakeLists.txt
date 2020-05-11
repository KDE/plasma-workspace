
set(systemmonitor_engine_SRCS
   systemmonitor.cpp
)

add_library(plasma_engine_systemmonitor MODULE ${systemmonitor_engine_SRCS})

target_link_libraries(plasma_engine_systemmonitor
    Qt5::Network
    KF5::I18n
    KF5::Plasma
    KF5::Service
    KSysGuard::SysGuard
)

kcoreaddons_desktop_to_json(plasma_engine_systemmonitor plasma-dataengine-systemmonitor.desktop)

install(TARGETS plasma_engine_systemmonitor DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-systemmonitor.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
