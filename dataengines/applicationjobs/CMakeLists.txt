add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_applicationjobs\")

set(kuiserver_engine_SRCS
    kuiserverengine.cpp
    jobcontrol.cpp
    jobaction.cpp
)

add_library(plasma_engine_applicationjobs MODULE ${kuiserver_engine_SRCS})
target_link_libraries(plasma_engine_applicationjobs
  Qt::DBus
  KF6::CoreAddons
  KF6::I18n
  KF6::KIOCore
  Plasma::Plasma
  KF6::Plasma5Support
  KF6::Service
  PW::LibNotificationManager
)

install(TARGETS plasma_engine_applicationjobs DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)
install(FILES applicationjobs.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
