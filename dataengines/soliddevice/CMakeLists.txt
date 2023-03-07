add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_soliddevice\")

set(soliddevice_engine_SRCS
    soliddeviceengine.cpp
    devicesignalmapper.cpp
    devicesignalmapmanager.cpp
    hddtemp.cpp
    soliddeviceservice.cpp
    soliddevicejob.cpp
    soliddeviceengine.h
    devicesignalmapper.h
    devicesignalmapmanager.h
    hddtemp.h
    soliddeviceservice.h
    soliddevicejob.h
)

kcoreaddons_add_plugin(plasma_engine_soliddevice SOURCES ${soliddevice_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_soliddevice
  Qt::Network
  KF6::I18n
  KF6::KIOCore
  KF6::Plasma
  KF6::Plasma5Support
  KF6::Solid
  KF6::CoreAddons
  KF6::Notifications
)

install(FILES soliddevice.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services )
