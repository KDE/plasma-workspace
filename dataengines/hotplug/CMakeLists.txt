set(hotplug_engine_SRCS
    hotplugengine.cpp
    hotplugservice.cpp
    hotplugjob.cpp
)

add_library(plasma_engine_hotplug MODULE ${hotplug_engine_SRCS})
target_link_libraries(plasma_engine_hotplug
                Qt5::DBus
		KF5::CoreAddons
		KF5::Plasma
		KF5::Solid
		KF5::Service
		KF5::KIOCore
		KF5::KIOWidgets
                )

kservice_desktop_to_json(plasma_engine_hotplug plasma-dataengine-hotplug.desktop)

install(TARGETS plasma_engine_hotplug DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-hotplug.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install(FILES hotplug.operations DESTINATION ${DATA_INSTALL_DIR}/plasma/services )
