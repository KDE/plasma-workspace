
set(systemmonitor_engine_SRCS
   systemmonitor.cpp
   systemmonitor.h
)

kcoreaddons_add_plugin(plasma_engine_systemmonitor SOURCES ${systemmonitor_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_systemmonitor
    Qt::Network
    KF6::I18n
    KF6::Plasma
    KF6::Plasma5Support
    KF6::Service
    KSysGuard::SysGuard
)
