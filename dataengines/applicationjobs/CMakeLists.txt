add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_applicationjobs\")

set(kuiserver_engine_SRCS
    kuiserverengine.cpp
    jobcontrol.cpp
    jobaction.cpp
)

qt5_add_dbus_adaptor(kuiserver_engine_SRCS ${CMAKE_SOURCE_DIR}/kuiserver/org.kde.JobView.xml kuiserverengine.h JobView jobviewadaptor )
qt5_add_dbus_adaptor(kuiserver_engine_SRCS ${KJOBWIDGETS_DBUS_INTERFACES_DIR}/kf5_org.kde.JobViewServer.xml kuiserverengine.h KuiserverEngine jobviewserveradaptor )

add_library(plasma_engine_applicationjobs MODULE ${kuiserver_engine_SRCS})
target_link_libraries(plasma_engine_applicationjobs
  Qt5::DBus
  KF5::CoreAddons
  KF5::I18n
  KF5::KIOCore
  KF5::Plasma
  KF5::Service
)

kcoreaddons_desktop_to_json(plasma_engine_applicationjobs plasma-dataengine-applicationjobs.desktop)

install(TARGETS plasma_engine_applicationjobs DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-applicationjobs.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install(FILES applicationjobs.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
