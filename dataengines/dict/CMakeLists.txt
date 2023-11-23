add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_dict\")

set(dict_engine_SRCS
    dictengine.cpp
    dictengine.h
)

add_library(plasma_engine_dict MODULE ${dict_engine_SRCS})

target_link_libraries (plasma_engine_dict
  Plasma::Plasma
  KF6::Plasma5Support
  KF6::Service
  KF6::I18n
  Qt::Network
)

install(TARGETS plasma_engine_dict DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)
