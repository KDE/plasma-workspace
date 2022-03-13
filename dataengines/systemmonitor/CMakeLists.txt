
set(systemmonitor_engine_SRCS
   systemmonitor.cpp
   systemmonitor.h
)

kcoreaddons_add_plugin(plasma_engine_systemmonitor SOURCES ${systemmonitor_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

target_link_libraries(plasma_engine_systemmonitor
    Qt::Network
    KF5::I18n
    KF5::Plasma
    KF5::Service
    KSysGuard::SysGuard
)
