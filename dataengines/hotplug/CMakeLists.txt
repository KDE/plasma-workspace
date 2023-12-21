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

kcoreaddons_add_plugin(plasma_engine_hotplug SOURCES ${hotplug_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)
target_link_libraries(plasma_engine_hotplug
    KF6::CoreAddons
    Plasma::Plasma
    Plasma::Plasma5Support
    KF6::Solid
    KF6::Service
    KF6::KIOCore
    KF6::KIOGui # KIO::CommandLauncherJob
    KF6::JobWidgets # KNotificationJobUiDelegate
    KF6::Notifications
    KF6::I18n
)

install(FILES hotplug.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
