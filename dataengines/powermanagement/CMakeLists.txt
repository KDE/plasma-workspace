add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_powermanagement\")

set(powermanagement_engine_SRCS
    powermanagementengine.cpp
    powermanagementjob.cpp
    powermanagementservice.cpp
    powermanagementengine.h
    powermanagementjob.h
    powermanagementservice.h
)

set(krunner_xml ${plasma-workspace_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml)
qt_add_dbus_interface(powermanagement_engine_SRCS ${krunner_xml} krunner_interface)

kcoreaddons_add_plugin(plasma_engine_powermanagement SOURCES ${powermanagement_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_powermanagement
    KF6::Solid
    KF6::Plasma
    KF6::Plasma5Support
    KF6::IdleTime
    KF6::CoreAddons
    KF6::I18n
    KF6::Service
    Qt::DBus
    PW::KWorkspace
)

install(FILES powermanagementservice.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
