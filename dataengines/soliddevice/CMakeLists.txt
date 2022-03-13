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

kcoreaddons_add_plugin(plasma_engine_soliddevice SOURCES ${soliddevice_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

target_link_libraries(plasma_engine_soliddevice
  Qt::Network
  KF5::I18n
  KF5::KIOCore
  KF5::Plasma
  KF5::Solid
  KF5::CoreAddons
  KF5::Notifications
)

install(FILES soliddevice.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services )
