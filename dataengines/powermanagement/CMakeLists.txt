add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_powermanagement\")

set(powermanagement_engine_SRCS
    powermanagementengine.cpp
    powermanagementjob.cpp
    powermanagementservice.cpp
)

set(krunner_xml ${plasma-workspace_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml)
qt5_add_dbus_interface(powermanagement_engine_SRCS ${krunner_xml} krunner_interface)

add_library(plasma_engine_powermanagement MODULE ${powermanagement_engine_SRCS})

target_link_libraries(plasma_engine_powermanagement
    KF5::Solid
    KF5::Plasma
    KF5::IdleTime
    KF5::CoreAddons
    KF5::I18n
    KF5::Service
    Qt::DBus
    PW::KWorkspace
)

kcoreaddons_desktop_to_json(plasma_engine_powermanagement plasma-dataengine-powermanagement.desktop)

install(TARGETS plasma_engine_powermanagement DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-powermanagement.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install(FILES powermanagementservice.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
