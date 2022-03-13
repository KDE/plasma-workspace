add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_dict\")

set(dict_engine_SRCS
    dictengine.cpp
    dictengine.h
)

add_library(plasma_engine_dict MODULE ${dict_engine_SRCS})

target_link_libraries (plasma_engine_dict
  KF5::Plasma
  KF5::Service
  KF5::I18n
  Qt::Network
)

install(TARGETS plasma_engine_dict DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
