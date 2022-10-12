add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_hotplug\")

set(hotplug_engine_SRCS
    deviceserviceaction.cpp
    hotplugengine.cpp
    hotplugservice.cpp
    hotplugjob.cpp
    deviceserviceaction.h
    hotplugengine.h
    hotplugservice.h
    hotplugjob.h
)

kcoreaddons_add_plugin(plasma_engine_hotplug SOURCES ${hotplug_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)
target_link_libraries(plasma_engine_hotplug
    KF5::CoreAddons
    KF5::Plasma
    KF5::Solid
    KF5::Service
    KF5::KIOCore
    KF5::KIOWidgets      # KDesktopFileActions
    KF5::Notifications
    KF5::I18n
)

install(FILES hotplug.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
